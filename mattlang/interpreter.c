#include "matt.h"
#include <stdarg.h>

static Context ctx;

// Forward declarations
static void exec_stmt(ASTNode *node);

// Helper function to copy a value
static Value copy_value(Value v) {
    Value copy = v;
    if (v.type == TYPE_STRING && v.value.str_val) {
        copy.value.str_val = strdup(v.value.str_val);
    }
    return copy;
}

// Scope management
static Scope *make_scope(Scope *parent) {
    Scope *scope = (Scope *)calloc(1, sizeof(Scope));
    scope->parent = parent;
    scope->symbols = NULL;
    return scope;
}

static void push_scope() {
    ctx.current_scope = make_scope(ctx.current_scope);
}

static void pop_scope() {
    if (!ctx.current_scope) return;
    Scope *old = ctx.current_scope;
    ctx.current_scope = old->parent;

    // Free symbols
    SymbolEntry *sym = old->symbols;
    while (sym) {
        SymbolEntry *next = sym->next;
        free(sym->name);
        free(sym->type);
        // TODO: Fix memory management - not freeing string values to avoid double-free
        // Free value data if needed
        // if (sym->value.type == TYPE_STRING && sym->value.value.str_val) {
        //     free(sym->value.value.str_val);
        // }
        free(sym);
        sym = next;
    }
    free(old);
}

static SymbolEntry *lookup_symbol(const char *name) {
    Scope *scope = ctx.current_scope;
    while (scope) {
        SymbolEntry *sym = scope->symbols;
        while (sym) {
            if (strcmp(sym->name, name) == 0) {
                return sym;
            }
            sym = sym->next;
        }
        scope = scope->parent;
    }
    return NULL;
}

static void define_symbol(const char *name, TypeInfo *type, Value value) {
    SymbolEntry *sym = (SymbolEntry *)calloc(1, sizeof(SymbolEntry));
    sym->name = strdup(name);
    sym->type = type;
    // Store value directly without copying to avoid memory issues
    sym->value = value;
    sym->is_initialized = true;
    sym->next = ctx.current_scope->symbols;
    ctx.current_scope->symbols = sym;
}

static Value make_int(int val) {
    Value v;
    v.type = TYPE_INT;
    v.value.int_val = val;
    return v;
}

static Value make_float(double val) {
    Value v;
    v.type = TYPE_FLOAT;
    v.value.float_val = val;
    return v;
}

static Value make_bool(bool val) {
    Value v;
    v.type = TYPE_BOOL;
    v.value.bool_val = val;
    return v;
}

static Value make_string(const char *val) {
    Value v;
    v.type = TYPE_STRING;
    v.value.str_val = val ? strdup(val) : NULL;
    return v;
}

static Value make_char(char val) {
    Value v;
    v.type = TYPE_CHAR;
    v.value.char_val = val;
    return v;
}

static Value make_array(DataType elem_type) {
    Value v;
    v.type = TYPE_ARRAY;
    v.value.array.elem_type = elem_type;
    v.value.array.length = 0;
    v.value.array.capacity = 8;
    v.value.array.elements = (void **)calloc(v.value.array.capacity, sizeof(void *));
    return v;
}

static void array_append(Value *array, Value elem) {
    if (array->value.array.length >= array->value.array.capacity) {
        array->value.array.capacity *= 2;
        array->value.array.elements = (void **)realloc(
            array->value.array.elements,
            array->value.array.capacity * sizeof(void *)
        );
    }
    Value *elem_copy = (Value *)malloc(sizeof(Value));
    *elem_copy = elem;
    array->value.array.elements[array->value.array.length++] = elem_copy;
}

static Value eval_expr(ASTNode *node);

static Value eval_literal(ASTNode *node) {
    if (node->data_type->base_type == TYPE_INT) {
        return make_int(node->data.literal.value.int_val);
    } else if (node->data_type->base_type == TYPE_FLOAT) {
        return make_float(node->data.literal.value.float_val);
    } else if (node->data_type->base_type == TYPE_BOOL) {
        return make_bool(node->data.literal.value.bool_val);
    } else if (node->data_type->base_type == TYPE_STRING) {
        return make_string(node->data.literal.value.str_val);
    } else if (node->data_type->base_type == TYPE_CHAR) {
        return make_char(node->data.literal.value.char_val);
    } else if (node->data_type->base_type == TYPE_NULL) {
        Value v;
        v.type = TYPE_NULL;
        return v;
    }

    fprintf(stderr, "Unknown literal type\n");
    exit(1);
}

static Value eval_identifier(ASTNode *node) {
    SymbolEntry *sym = lookup_symbol(node->data.identifier.name);
    if (!sym) {
        fprintf(stderr, "Undefined variable: %s\n", node->data.identifier.name);
        exit(1);
    }
    // Return value directly without copying to avoid memory issues
    return sym->value;
}

static Value eval_binary_op(ASTNode *node) {
    Value left = eval_expr(node->data.binary.left);
    Value right = eval_expr(node->data.binary.right);

    switch (node->data.binary.op) {
        case TOKEN_PLUS:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                return make_int(left.value.int_val + right.value.int_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                return make_float(l + r);
            }
            break;

        case TOKEN_MINUS:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                return make_int(left.value.int_val - right.value.int_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                return make_float(l - r);
            }
            break;

        case TOKEN_STAR:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                return make_int(left.value.int_val * right.value.int_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                return make_float(l * r);
            }
            break;

        case TOKEN_SLASH:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                if (right.value.int_val == 0) {
                    fprintf(stderr, "Division by zero\n");
                    exit(1);
                }
                return make_int(left.value.int_val / right.value.int_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                if (r == 0.0) {
                    fprintf(stderr, "Division by zero\n");
                    exit(1);
                }
                return make_float(l / r);
            }
            break;

        case TOKEN_PERCENT:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                if (right.value.int_val == 0) {
                    fprintf(stderr, "Modulo by zero\n");
                    exit(1);
                }
                return make_int(left.value.int_val % right.value.int_val);
            }
            break;

        case TOKEN_LT:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                return make_bool(left.value.int_val < right.value.int_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                return make_bool(l < r);
            }
            break;

        case TOKEN_GT:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                return make_bool(left.value.int_val > right.value.int_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                return make_bool(l > r);
            }
            break;

        case TOKEN_LTE:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                return make_bool(left.value.int_val <= right.value.int_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                return make_bool(l <= r);
            }
            break;

        case TOKEN_GTE:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                return make_bool(left.value.int_val >= right.value.int_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                return make_bool(l >= r);
            }
            break;

        case TOKEN_EQ:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                return make_bool(left.value.int_val == right.value.int_val);
            } else if (left.type == TYPE_BOOL && right.type == TYPE_BOOL) {
                return make_bool(left.value.bool_val == right.value.bool_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                return make_bool(l == r);
            }
            break;

        case TOKEN_NEQ:
            if (left.type == TYPE_INT && right.type == TYPE_INT) {
                return make_bool(left.value.int_val != right.value.int_val);
            } else if (left.type == TYPE_BOOL && right.type == TYPE_BOOL) {
                return make_bool(left.value.bool_val != right.value.bool_val);
            } else if (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT) {
                double l = (left.type == TYPE_FLOAT) ? left.value.float_val : left.value.int_val;
                double r = (right.type == TYPE_FLOAT) ? right.value.float_val : right.value.int_val;
                return make_bool(l != r);
            }
            break;

        case TOKEN_AND:
            return make_bool(left.value.bool_val && right.value.bool_val);

        case TOKEN_OR:
            return make_bool(left.value.bool_val || right.value.bool_val);

        default:
            break;
    }

    fprintf(stderr, "Invalid binary operation\n");
    exit(1);
}

static Value eval_unary_op(ASTNode *node) {
    Value operand = eval_expr(node->data.unary.operand);

    switch (node->data.unary.op) {
        case TOKEN_MINUS:
            if (operand.type == TYPE_INT) {
                return make_int(-operand.value.int_val);
            } else if (operand.type == TYPE_FLOAT) {
                return make_float(-operand.value.float_val);
            }
            break;

        case TOKEN_NOT:
            if (operand.type == TYPE_BOOL) {
                return make_bool(!operand.value.bool_val);
            }
            break;

        default:
            break;
    }

    fprintf(stderr, "Invalid unary operation\n");
    exit(1);
}

static Value eval_cast(ASTNode *node) {
    Value val = eval_expr(node->data.cast.expr);
    DataType target = node->data.cast.target_type->base_type;

    if (val.type == TYPE_INT) {
        if (target == TYPE_FLOAT) {
            return make_float((double)val.value.int_val);
        } else if (target == TYPE_BOOL) {
            return make_bool(val.value.int_val != 0);
        }
    } else if (val.type == TYPE_FLOAT) {
        if (target == TYPE_INT) {
            return make_int((int)val.value.float_val);
        }
    }

    return val; // No conversion needed
}

static Value eval_array_literal(ASTNode *node) {
    if (node->data.array_literal.elem_count == 0) {
        return make_array(TYPE_INT); // Default to int array
    }

    Value first = eval_expr(node->data.array_literal.elements[0]);
    Value array = make_array(first.type);

    for (int i = 0; i < node->data.array_literal.elem_count; i++) {
        Value elem = eval_expr(node->data.array_literal.elements[i]);
        array_append(&array, elem);
    }

    return array;
}

static Value eval_array_access(ASTNode *node) {
    Value array = eval_expr(node->data.array_access.array);
    Value index = eval_expr(node->data.array_access.index);

    if (array.type != TYPE_ARRAY) {
        fprintf(stderr, "Cannot index non-array type\n");
        exit(1);
    }

    if (index.type != TYPE_INT) {
        fprintf(stderr, "Array index must be an integer\n");
        exit(1);
    }

    int idx = index.value.int_val;
    if (idx < 0 || idx >= array.value.array.length) {
        fprintf(stderr, "Array index out of bounds: %d (length: %d)\n",
                idx, array.value.array.length);
        exit(1);
    }

    return *(Value *)array.value.array.elements[idx];
}

static Value eval_member_access(ASTNode *node) {
    Value obj = eval_expr(node->data.member_access.object);

    if (obj.type == TYPE_ARRAY && strcmp(node->data.member_access.member, "length") == 0) {
        return make_int(obj.value.array.length);
    }

    fprintf(stderr, "Unknown member: %s\n", node->data.member_access.member);
    exit(1);
}

static void matt_printf(const char *format, Value *args, int arg_count);

static Value eval_call(ASTNode *node) {
    // Handle printf specially
    if (strcmp(node->data.call.name, "printf") == 0) {
        if (node->data.call.arg_count == 0) {
            fprintf(stderr, "printf requires at least one argument\n");
            exit(1);
        }

        Value format_val = eval_expr(node->data.call.args[0]);
        if (format_val.type != TYPE_STRING) {
            fprintf(stderr, "printf format must be a string\n");
            exit(1);
        }

        Value *args = NULL;
        if (node->data.call.arg_count > 1) {
            args = (Value *)malloc(sizeof(Value) * (node->data.call.arg_count - 1));
            for (int i = 1; i < node->data.call.arg_count; i++) {
                args[i - 1] = eval_expr(node->data.call.args[i]);
            }
        }

        matt_printf(format_val.value.str_val, args, node->data.call.arg_count - 1);

        if (args) free(args);

        Value v;
        v.type = TYPE_VOID;
        return v;
    }

    // Look up function in global scope
    SymbolEntry *func_sym = lookup_symbol(node->data.call.name);
    if (!func_sym) {
        fprintf(stderr, "Undefined function: %s\n", node->data.call.name);
        exit(1);
    }

    // Find function AST node
    ASTNode *func_node = (ASTNode *)func_sym->value.value.str_val; // Hack: store function ptr

    // Create new scope for function
    push_scope();

    // Bind parameters
    for (int i = 0; i < func_node->data.function.param_count; i++) {
        Value arg = eval_expr(node->data.call.args[i]);
        define_symbol(func_node->data.function.param_names[i],
                     func_node->data.function.param_types[i], arg);
    }

    // Execute function body
    ctx.should_return = false;
    exec_stmt(func_node->data.function.body);

    Value return_val = ctx.return_value;
    pop_scope();

    return return_val;
}

static Value eval_assign(ASTNode *node) {
    Value val = eval_expr(node->data.assign.value);

    if (node->data.assign.target->type == NODE_IDENTIFIER) {
        SymbolEntry *sym = lookup_symbol(node->data.assign.target->data.identifier.name);
        if (!sym) {
            fprintf(stderr, "Undefined variable: %s\n",
                    node->data.assign.target->data.identifier.name);
            exit(1);
        }
        // Store value directly without copying
        sym->value = val;
    } else if (node->data.assign.target->type == NODE_ARRAY_ACCESS) {
        Value array = eval_expr(node->data.assign.target->data.array_access.array);
        Value index = eval_expr(node->data.assign.target->data.array_access.index);

        if (array.type != TYPE_ARRAY) {
            fprintf(stderr, "Cannot index non-array type\n");
            exit(1);
        }

        int idx = index.value.int_val;
        if (idx < 0 || idx >= array.value.array.length) {
            fprintf(stderr, "Array index out of bounds\n");
            exit(1);
        }

        *(Value *)array.value.array.elements[idx] = val;
    }

    return val;
}

static Value eval_expr(ASTNode *node) {
    if (!node) {
        Value v;
        v.type = TYPE_VOID;
        return v;
    }

    switch (node->type) {
        case NODE_LITERAL:
            return eval_literal(node);
        case NODE_IDENTIFIER:
            return eval_identifier(node);
        case NODE_BINARY_OP:
            return eval_binary_op(node);
        case NODE_UNARY_OP:
            return eval_unary_op(node);
        case NODE_CAST:
            return eval_cast(node);
        case NODE_ARRAY_LITERAL:
            return eval_array_literal(node);
        case NODE_ARRAY_ACCESS:
            return eval_array_access(node);
        case NODE_MEMBER_ACCESS:
            return eval_member_access(node);
        case NODE_CALL:
            return eval_call(node);
        case NODE_ASSIGN:
            return eval_assign(node);
        default:
            fprintf(stderr, "Unknown expression node type: %d\n", node->type);
            exit(1);
    }
}

static void exec_block(ASTNode *node) {
    push_scope();
    for (int i = 0; i < node->data.block.stmt_count; i++) {
        if (ctx.should_return || ctx.should_break || ctx.should_continue) {
            break;
        }
        exec_stmt(node->data.block.statements[i]);
    }
    pop_scope();
}

static void exec_var_decl(ASTNode *node) {
    Value val = eval_expr(node->data.var_decl.initializer);
    define_symbol(node->data.var_decl.name, node->data.var_decl.var_type, val);
}

static void exec_if(ASTNode *node) {
    Value condition = eval_expr(node->data.if_stmt.condition);
    if (condition.type != TYPE_BOOL) {
        fprintf(stderr, "If condition must be a boolean\n");
        exit(1);
    }

    if (condition.value.bool_val) {
        exec_stmt(node->data.if_stmt.then_branch);
    } else if (node->data.if_stmt.else_branch) {
        exec_stmt(node->data.if_stmt.else_branch);
    }
}

static void exec_while(ASTNode *node) {
    bool prev_in_loop = ctx.in_loop;
    ctx.in_loop = true;

    while (true) {
        Value condition = eval_expr(node->data.while_stmt.condition);
        if (condition.type != TYPE_BOOL) {
            fprintf(stderr, "While condition must be a boolean\n");
            exit(1);
        }

        if (!condition.value.bool_val) {
            break;
        }

        exec_stmt(node->data.while_stmt.body);

        if (ctx.should_break) {
            ctx.should_break = false;
            break;
        }

        if (ctx.should_continue) {
            ctx.should_continue = false;
            continue;
        }

        if (ctx.should_return) {
            break;
        }
    }

    ctx.in_loop = prev_in_loop;
}

static void exec_for(ASTNode *node) {
    bool prev_in_loop = ctx.in_loop;
    ctx.in_loop = true;

    push_scope(); // For loop init scope

    if (node->data.for_stmt.init) {
        if (node->data.for_stmt.init->type == NODE_VAR_DECL) {
            exec_var_decl(node->data.for_stmt.init);
        } else {
            eval_expr(node->data.for_stmt.init);
        }
    }

    while (true) {
        if (node->data.for_stmt.condition) {
            Value condition = eval_expr(node->data.for_stmt.condition);
            if (condition.type != TYPE_BOOL) {
                fprintf(stderr, "For condition must be a boolean\n");
                exit(1);
            }

            if (!condition.value.bool_val) {
                break;
            }
        }

        exec_stmt(node->data.for_stmt.body);

        if (ctx.should_break) {
            ctx.should_break = false;
            break;
        }

        if (ctx.should_continue) {
            ctx.should_continue = false;
        }

        if (ctx.should_return) {
            break;
        }

        if (node->data.for_stmt.increment) {
            eval_expr(node->data.for_stmt.increment);
        }
    }

    pop_scope();
    ctx.in_loop = prev_in_loop;
}

static void exec_return(ASTNode *node) {
    if (node->data.return_stmt.value) {
        ctx.return_value = eval_expr(node->data.return_stmt.value);
    } else {
        ctx.return_value.type = TYPE_VOID;
    }
    ctx.should_return = true;
}

static void exec_stmt(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_BLOCK:
            exec_block(node);
            break;
        case NODE_VAR_DECL:
            exec_var_decl(node);
            break;
        case NODE_IF:
            exec_if(node);
            break;
        case NODE_WHILE:
            exec_while(node);
            break;
        case NODE_FOR:
            exec_for(node);
            break;
        case NODE_RETURN:
            exec_return(node);
            break;
        case NODE_BREAK:
            if (!ctx.in_loop) {
                fprintf(stderr, "Break outside of loop\n");
                exit(1);
            }
            ctx.should_break = true;
            break;
        case NODE_CONTINUE:
            if (!ctx.in_loop) {
                fprintf(stderr, "Continue outside of loop\n");
                exit(1);
            }
            ctx.should_continue = true;
            break;
        case NODE_EXPR_STMT:
            eval_expr(node->data.expr_stmt.expr);
            break;
        default:
            fprintf(stderr, "Unknown statement node type: %d\n", node->type);
            exit(1);
    }
}

static void matt_printf(const char *format, Value *args, int arg_count) {
    int arg_idx = 0;
    for (const char *p = format; *p; p++) {
        if (*p == '%' && *(p + 1)) {
            p++;
            if (*p == 'd' || *p == 'i') {
                if (arg_idx >= arg_count) {
                    fprintf(stderr, "Not enough arguments for printf\n");
                    exit(1);
                }
                printf("%d", args[arg_idx].value.int_val);
                arg_idx++;
            } else if (*p == 'f' || *p == 'g') {
                if (arg_idx >= arg_count) {
                    fprintf(stderr, "Not enough arguments for printf\n");
                    exit(1);
                }
                printf("%g", args[arg_idx].value.float_val);
                arg_idx++;
            } else if (*p == 's') {
                if (arg_idx >= arg_count) {
                    fprintf(stderr, "Not enough arguments for printf\n");
                    exit(1);
                }
                printf("%s", args[arg_idx].value.str_val);
                arg_idx++;
            } else if (*p == 'c') {
                if (arg_idx >= arg_count) {
                    fprintf(stderr, "Not enough arguments for printf\n");
                    exit(1);
                }
                printf("%c", args[arg_idx].value.char_val);
                arg_idx++;
            } else if (*p == '%') {
                printf("%%");
            } else {
                printf("%%%c", *p);
            }
        } else {
            printf("%c", *p);
        }
    }
}

Value interpret(ASTNode *ast) {
    ctx.global_scope = make_scope(NULL);
    ctx.current_scope = ctx.global_scope;
    ctx.in_loop = false;
    ctx.should_break = false;
    ctx.should_continue = false;
    ctx.should_return = false;

    // Register all functions in global scope
    for (int i = 0; i < ast->data.program.func_count; i++) {
        ASTNode *func = ast->data.program.functions[i];
        Value func_val;
        func_val.type = TYPE_VOID;
        func_val.value.str_val = (char *)func; // Store function node pointer
        define_symbol(func->data.function.name, func->data.function.return_type, func_val);
    }

    // Find and execute main function
    SymbolEntry *main_sym = lookup_symbol("main");
    if (!main_sym) {
        fprintf(stderr, "No main function found\n");
        exit(1);
    }

    ASTNode *main_func = (ASTNode *)main_sym->value.value.str_val;

    ctx.should_return = false;
    exec_stmt(main_func->data.function.body);

    return ctx.return_value;
}
