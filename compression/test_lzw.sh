#!/bin/bash

# LZW Compression Test Suite
# Tests the LZW compression utility with various test cases

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_PASSED=0
TESTS_FAILED=0
TOTAL_TESTS=0

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LZW_BINARY="$SCRIPT_DIR/lzw"
TEST_DIR="/tmp/lzw_tests_$$"

# Compile the LZW program if needed
if [ ! -f "$LZW_BINARY" ]; then
    echo -e "${YELLOW}Compiling LZW compression program...${NC}"
    gcc -o "$LZW_BINARY" "$SCRIPT_DIR/lzw.c" -Wall
    echo -e "${GREEN}Compilation successful!${NC}\n"
fi

# Create test directory
mkdir -p "$TEST_DIR"

# Cleanup function
cleanup() {
    rm -rf "$TEST_DIR"
}
trap cleanup EXIT

# Test result function
test_result() {
    local test_name="$1"
    local status="$2"
    local details="$3"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    if [ "$status" = "PASS" ]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "${GREEN}✓ PASS${NC} - $test_name"
        if [ -n "$details" ]; then
            echo -e "  ${BLUE}$details${NC}"
        fi
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "${RED}✗ FAIL${NC} - $test_name"
        if [ -n "$details" ]; then
            echo -e "  ${RED}$details${NC}"
        fi
    fi
    echo
}

# Function to run compression and check result
run_compression_test() {
    local test_name="$1"
    local input_file="$2"
    local expected_result="$3"  # "should_exist", "should_compress", "should_expand"

    local output_file="$TEST_DIR/output.lzw"

    # Run compression
    if "$LZW_BINARY" "$input_file" "$output_file" > "$TEST_DIR/lzw_output.txt" 2>&1; then
        if [ ! -f "$output_file" ]; then
            test_result "$test_name" "FAIL" "Output file was not created"
            return 1
        fi

        local input_size=$(stat -f%z "$input_file" 2>/dev/null || stat -c%s "$input_file" 2>/dev/null)
        local output_size=$(stat -f%z "$output_file" 2>/dev/null || stat -c%s "$output_file" 2>/dev/null)

        case "$expected_result" in
            "should_exist")
                test_result "$test_name" "PASS" "Input: ${input_size}B, Output: ${output_size}B"
                ;;
            "should_compress")
                if [ "$output_size" -lt "$input_size" ]; then
                    local ratio=$(awk "BEGIN {printf \"%.2f\", ($output_size/$input_size)*100}")
                    test_result "$test_name" "PASS" "Compressed ${input_size}B → ${output_size}B (${ratio}%)"
                else
                    test_result "$test_name" "FAIL" "File did not compress: ${input_size}B → ${output_size}B"
                fi
                ;;
            "should_expand")
                if [ "$output_size" -ge "$input_size" ]; then
                    test_result "$test_name" "PASS" "Expected expansion: ${input_size}B → ${output_size}B"
                else
                    test_result "$test_name" "FAIL" "Unexpected compression: ${input_size}B → ${output_size}B"
                fi
                ;;
        esac
    else
        test_result "$test_name" "FAIL" "Compression command failed"
        return 1
    fi
}

echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}LZW COMPRESSION TEST SUITE${NC}"
echo -e "${BLUE}================================${NC}\n"

# TEST 1: Empty file
echo -e "${YELLOW}Test 1: Empty file${NC}"
touch "$TEST_DIR/empty.txt"
run_compression_test "Empty file" "$TEST_DIR/empty.txt" "should_exist"

# TEST 2: Single character
echo -e "${YELLOW}Test 2: Single character${NC}"
echo -n "A" > "$TEST_DIR/single.txt"
run_compression_test "Single character" "$TEST_DIR/single.txt" "should_exist"

# TEST 3: Highly repetitive pattern (should compress well)
echo -e "${YELLOW}Test 3: Highly repetitive pattern${NC}"
python3 -c "print('ABABABABAB' * 100)" > "$TEST_DIR/repetitive.txt"
run_compression_test "Highly repetitive pattern" "$TEST_DIR/repetitive.txt" "should_compress"

# TEST 4: Classic LZW test string
echo -e "${YELLOW}Test 4: Classic LZW test string${NC}"
echo -n "TOBEORNOTTOBEORTOBEORNOT" > "$TEST_DIR/classic.txt"
run_compression_test "Classic LZW string" "$TEST_DIR/classic.txt" "should_exist"

# TEST 5: Random data (unlikely to compress well)
echo -e "${YELLOW}Test 5: Random data${NC}"
head -c 100 /dev/urandom | base64 > "$TEST_DIR/random.txt"
run_compression_test "Random data" "$TEST_DIR/random.txt" "should_expand"

# TEST 6: Repeated words with spaces
echo -e "${YELLOW}Test 6: Repeated words with spaces${NC}"
python3 -c "print(' '.join(['hello', 'world'] * 200))" > "$TEST_DIR/words.txt"
run_compression_test "Repeated words" "$TEST_DIR/words.txt" "should_compress"

# TEST 7: Numbers with patterns
echo -e "${YELLOW}Test 7: Numbers with patterns${NC}"
python3 -c "print(''.join(['123456789'] * 100))" > "$TEST_DIR/numbers.txt"
run_compression_test "Repeated numbers" "$TEST_DIR/numbers.txt" "should_compress"

# TEST 8: ASCII art pattern
echo -e "${YELLOW}Test 8: ASCII art pattern${NC}"
cat > "$TEST_DIR/ascii_art.txt" << 'EOF'
********
*      *
*      *
********
********
*      *
*      *
********
EOF
python3 -c "with open('$TEST_DIR/ascii_art.txt', 'r') as f: content = f.read(); print(content * 50)" > "$TEST_DIR/ascii_repeated.txt"
run_compression_test "ASCII art pattern" "$TEST_DIR/ascii_repeated.txt" "should_compress"

# TEST 9: JSON-like structured data with repetition
echo -e "${YELLOW}Test 9: JSON-like structured data${NC}"
python3 << 'PYTHON_EOF' > "$TEST_DIR/json_like.txt"
for i in range(100):
    print(f'{{"id": {i}, "name": "user{i}", "status": "active", "type": "standard"}}')
PYTHON_EOF
run_compression_test "JSON-like data" "$TEST_DIR/json_like.txt" "should_compress"

# TEST 10: Mixed case text
echo -e "${YELLOW}Test 10: Mixed case text${NC}"
cat > "$TEST_DIR/mixed.txt" << 'EOF'
The quick brown fox jumps over the lazy dog.
The quick brown fox jumps over the lazy dog.
The quick brown fox jumps over the lazy dog.
EOF
python3 -c "with open('$TEST_DIR/mixed.txt', 'r') as f: content = f.read(); print(content * 50)" > "$TEST_DIR/mixed_repeated.txt"
run_compression_test "Mixed case text" "$TEST_DIR/mixed_repeated.txt" "should_compress"

# TEST 11: Binary-like data with patterns
echo -e "${YELLOW}Test 11: Binary patterns (0s and 1s)${NC}"
python3 -c "print('01010101' * 500)" > "$TEST_DIR/binary.txt"
run_compression_test "Binary pattern" "$TEST_DIR/binary.txt" "should_compress"

# TEST 12: Large file with repetitive XML-like structure
echo -e "${YELLOW}Test 12: Large XML-like structure${NC}"
python3 << 'PYTHON_EOF' > "$TEST_DIR/xml_like.txt"
print("<root>")
for i in range(200):
    print(f"  <item><id>{i}</id><value>data</value><status>ok</status></item>")
print("</root>")
PYTHON_EOF
run_compression_test "Large XML-like structure" "$TEST_DIR/xml_like.txt" "should_compress"

# TEST 13: Special characters
echo -e "${YELLOW}Test 13: Special characters${NC}"
python3 -c "print('!@#$%^&*()_+-=[]{}|;:,.<>?' * 100)" > "$TEST_DIR/special.txt"
run_compression_test "Special characters" "$TEST_DIR/special.txt" "should_compress"

# TEST 14: Very long repeated substring
echo -e "${YELLOW}Test 14: Very long repeated substring${NC}"
python3 -c "substring = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' * 10; print(substring * 20)" > "$TEST_DIR/long_pattern.txt"
run_compression_test "Very long repeated pattern" "$TEST_DIR/long_pattern.txt" "should_compress"

# Print summary
echo -e "${BLUE}================================${NC}"
echo -e "${BLUE}TEST SUMMARY${NC}"
echo -e "${BLUE}================================${NC}"
echo -e "Total tests: ${TOTAL_TESTS}"
echo -e "${GREEN}Passed: ${TESTS_PASSED}${NC}"
echo -e "${RED}Failed: ${TESTS_FAILED}${NC}"
echo

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed! ✓${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
