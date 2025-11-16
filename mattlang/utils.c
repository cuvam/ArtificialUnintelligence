#include "matt.h"

TypeInfo *make_type(DataType base_type) {
    TypeInfo *type = (TypeInfo *)calloc(1, sizeof(TypeInfo));
    type->base_type = base_type;
    type->is_pointer = false;
    type->element_type = NULL;
    return type;
}

TypeInfo *make_array_type(DataType elem_type) {
    TypeInfo *type = (TypeInfo *)calloc(1, sizeof(TypeInfo));
    type->base_type = TYPE_ARRAY;
    type->is_pointer = false;
    type->element_type = make_type(elem_type);
    return type;
}

bool types_equal(TypeInfo *a, TypeInfo *b) {
    if (!a || !b) return false;
    if (a->base_type != b->base_type) return false;
    if (a->is_pointer != b->is_pointer) return false;

    if (a->base_type == TYPE_ARRAY) {
        return types_equal(a->element_type, b->element_type);
    }

    return true;
}

const char *type_to_string(TypeInfo *type) {
    if (!type) return "unknown";

    static char buffer[256];

    if (type->base_type == TYPE_ARRAY && type->element_type) {
        snprintf(buffer, sizeof(buffer), "%s[]", type_to_string(type->element_type));
        return buffer;
    }

    switch (type->base_type) {
        case TYPE_INT: return type->is_pointer ? "int*" : "int";
        case TYPE_FLOAT: return type->is_pointer ? "float*" : "float";
        case TYPE_DOUBLE: return type->is_pointer ? "double*" : "double";
        case TYPE_LONG: return type->is_pointer ? "long*" : "long";
        case TYPE_BOOL: return type->is_pointer ? "bool*" : "bool";
        case TYPE_CHAR: return type->is_pointer ? "char*" : "char";
        case TYPE_STRING: return "string";
        case TYPE_VOID: return "void";
        case TYPE_NULL: return "null";
        default: return "unknown";
    }
}

void print_value(Value v) {
    switch (v.type) {
        case TYPE_INT:
            printf("%d", v.value.int_val);
            break;
        case TYPE_FLOAT:
        case TYPE_DOUBLE:
            printf("%g", v.value.float_val);
            break;
        case TYPE_LONG:
            printf("%ld", v.value.long_val);
            break;
        case TYPE_BOOL:
            printf("%s", v.value.bool_val ? "true" : "false");
            break;
        case TYPE_CHAR:
            printf("%c", v.value.char_val);
            break;
        case TYPE_STRING:
            printf("%s", v.value.str_val ? v.value.str_val : "(null)");
            break;
        case TYPE_ARRAY:
            printf("[");
            for (int i = 0; i < v.value.array.length; i++) {
                if (i > 0) printf(", ");
                Value *elem = (Value *)v.value.array.elements[i];
                print_value(*elem);
            }
            printf("]");
            break;
        default:
            printf("(unknown)");
            break;
    }
}

void free_ast_node(ASTNode *node);

void free_ast(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            for (int i = 0; i < node->data.program.func_count; i++) {
                free_ast(node->data.program.functions[i]);
            }
            free(node->data.program.functions);
            break;

        case NODE_FUNCTION:
            free(node->data.function.name);
            for (int i = 0; i < node->data.function.param_count; i++) {
                free(node->data.function.param_names[i]);
                free(node->data.function.param_types[i]);
            }
            free(node->data.function.param_names);
            free(node->data.function.param_types);
            free(node->data.function.return_type);
            free_ast(node->data.function.body);
            break;

        case NODE_BLOCK:
            for (int i = 0; i < node->data.block.stmt_count; i++) {
                free_ast(node->data.block.statements[i]);
            }
            free(node->data.block.statements);
            break;

        case NODE_VAR_DECL:
            free(node->data.var_decl.name);
            free(node->data.var_decl.var_type);
            free_ast(node->data.var_decl.initializer);
            break;

        case NODE_RETURN:
            free_ast(node->data.return_stmt.value);
            break;

        case NODE_IF:
            free_ast(node->data.if_stmt.condition);
            free_ast(node->data.if_stmt.then_branch);
            free_ast(node->data.if_stmt.else_branch);
            break;

        case NODE_WHILE:
            free_ast(node->data.while_stmt.condition);
            free_ast(node->data.while_stmt.body);
            break;

        case NODE_FOR:
            free_ast(node->data.for_stmt.init);
            free_ast(node->data.for_stmt.condition);
            free_ast(node->data.for_stmt.increment);
            free_ast(node->data.for_stmt.body);
            break;

        case NODE_EXPR_STMT:
            free_ast(node->data.expr_stmt.expr);
            break;

        case NODE_BINARY_OP:
            free_ast(node->data.binary.left);
            free_ast(node->data.binary.right);
            break;

        case NODE_UNARY_OP:
            free_ast(node->data.unary.operand);
            break;

        case NODE_ASSIGN:
            free_ast(node->data.assign.target);
            free_ast(node->data.assign.value);
            break;

        case NODE_CALL:
            free(node->data.call.name);
            for (int i = 0; i < node->data.call.arg_count; i++) {
                free_ast(node->data.call.args[i]);
            }
            free(node->data.call.args);
            break;

        case NODE_ARRAY_ACCESS:
            free_ast(node->data.array_access.array);
            free_ast(node->data.array_access.index);
            break;

        case NODE_ARRAY_LITERAL:
            for (int i = 0; i < node->data.array_literal.elem_count; i++) {
                free_ast(node->data.array_literal.elements[i]);
            }
            free(node->data.array_literal.elements);
            break;

        case NODE_MEMBER_ACCESS:
            free_ast(node->data.member_access.object);
            free(node->data.member_access.member);
            break;

        case NODE_CAST:
            free(node->data.cast.target_type);
            free_ast(node->data.cast.expr);
            break;

        case NODE_LITERAL:
            if (node->data_type && node->data_type->base_type == TYPE_STRING) {
                free(node->data.literal.value.str_val);
            }
            break;

        case NODE_IDENTIFIER:
            free(node->data.identifier.name);
            break;

        default:
            break;
    }

    if (node->data_type) {
        free(node->data_type);
    }
    free(node);
}
