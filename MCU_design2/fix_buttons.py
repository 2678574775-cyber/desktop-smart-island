#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Fix maikefeng.ino button scanning code

import re

FILE = r"C:\Users\Gao Ruiyang\Desktop\Arduino\maikefeng\maikefeng.ino"

with open(FILE, 'r', encoding='utf-8', errors='replace') as f:
    content = f.read()

original_len = len(content)

# ================================================================
# Fix 1: Remove duplicate scanAddr definition
# ================================================================
# There are two scanAddr definitions. Remove the second one.
# The pattern: const uint8_t scanAddr[BTN_COUNT] = { 0b101, 0b000, 0b011, 0b100, 0b010, 0b001 };
# appears twice. Remove the one that appears after "HC138 address for each logical button [A,B,C,D,E,F]"
# The first one (line ~208) is the correct one. The second one is inside the duplicated block.

# Find all occurrences
occurrences = [m.start() for m in re.finditer(r'const uint8_t scanAddr\[BTN_COUNT\]', content)]
print(f"scanAddr definitions found: {len(occurrences)}")
if len(occurrences) > 1:
    # Remove the second one (and its preceding comment line)
    # Find the line with the second occurrence and remove it
    second_start = occurrences[1]
    # Find the start of the line (including preceding comment)
    line_start = content.rfind('\n', 0, second_start) + 1
    # Find the end of the line
    line_end = content.find('\n', second_start)
    if line_end == -1:
        line_end = len(content)
    else:
        line_end += 1  # include the newline
    
    # Also remove the comment line before it if it exists
    comment_start = content.rfind('\n', 0, line_start)
    if comment_start == -1:
        comment_start = 0
    else:
        comment_start += 1
    comment_line_end = content.find('\n', comment_start)
    comment_text = content[comment_start:comment_line_end]
    if 'HC138 address for each logical button' in comment_text or 'HC138 address for each' in comment_text:
        # Remove comment line + the scanAddr line
        content = content[:comment_start] + content[line_end:]
    else:
        # Just remove the scanAddr line
        content = content[:line_start] + content[line_end:]
    print(f"  Removed duplicate scanAddr definition")

# ================================================================
# Fix 2: Remove static prevRead definition inside function
# ================================================================
# static uint8_t prevRead[BTN_COUNT]; inside updateButtons() conflicts with global
occurrences = [m.start() for m in re.finditer(r'static uint8_t prevRead\[BTN_COUNT\]', content)]
print(f"static prevRead definitions found: {len(occurrences)}")
if occurrences:
    for pos in reversed(occurrences):
        line_start = content.rfind('\n', 0, pos) + 1
        line_end = content.find('\n', pos)
        if line_end == -1:
            line_end = len(content)
        else:
            line_end += 1
        content = content[:line_start] + content[line_end:]
    print(f"  Removed {len(occurrences)} static prevRead definition(s)")

# ================================================================
# Fix 3: Replace setHC138Addr with digitalWrite version
# ================================================================
old_setHC138 = """static inline void setHC138Addr(uint8_t addr) {
  uint32_t mask = (1<<0) | (1<<1) | (1<<2);  // GPIO0,1,2
  uint32_t val  = ((addr & 1)<<0) | (((addr>>1)&1)<<1) | (((addr>>2)&1)<<2);
  GPIO_OUT_W1TC_REG = mask;   // clear all 3 bits at once
  GPIO_OUT_W1TS_REG = val;    // set new bits at once
}"""

new_setHC138 = """static inline void setHC138Addr(uint8_t addr) {
  // Write all 3 address bits atomically (as atomic as digitalWrite allows)
  // The 30us settle time + counter debounce filters any transient glitches.
  digitalWrite(KEY_A0, (addr & 0x01) ? HIGH : LOW);
  digitalWrite(KEY_A1, (addr & 0x02) ? HIGH : LOW);
  digitalWrite(KEY_A2, (addr & 0x04) ? HIGH : LOW);
}"""

if old_setHC138 in content:
    content = content.replace(old_setHC138, new_setHC138)
    print("  Replaced setHC138Addr with digitalWrite version")
elif 'GPIO_OUT_W1TC_REG' in content:
    # Try a more flexible replacement
    # Use regex to match the function
    pattern = r'static inline void setHC138Addr\(uint8_t addr\) \{[^}]+\}'
    replacement = """static inline void setHC138Addr(uint8_t addr) {
  digitalWrite(KEY_A0, (addr & 0x01) ? HIGH : LOW);
  digitalWrite(KEY_A1, (addr & 0x02) ? HIGH : LOW);
  digitalWrite(KEY_A2, (addr & 0x04) ? HIGH : LOW);
}"""
    new_content = re.sub(pattern, replacement, content, flags=re.DOTALL)
    if new_content != content:
        content = new_content
        print("  Replaced setHC138Addr using regex")
    else:
        print("  WARNING: Could not find setHC138Addr to replace")

# ================================================================
# Fix 4: Remove BTN_DEBOUNCE_CNT and BTN_DB_CNT inconsistency
# ================================================================
# Make sure we use one consistent name
if 'BTN_DEBOUNCE_CNT' in content and 'BTN_DB_CNT' in content:
    content = content.replace('BTN_DB_CNT', 'BTN_DEBOUNCE_CNT')
    print("  Replaced BTN_DB_CNT -> BTN_DEBOUNCE_CNT")
elif 'BTN_DEBOUNCE_CNT' not in content and 'BTN_DB_CNT' in content:
    content = content.replace('BTN_DB_CNT', 'BTN_DEBOUNCE_CNT')
    print("  Replaced BTN_DB_CNT -> BTN_DEBOUNCE_CNT")

# ================================================================
# Fix 5: Fix btnDebounceCnt -> btnCnt naming
# ================================================================
# The clean code uses btnCnt, but previous edits might use btnDebounceCnt
# Make sure the variable names are consistent
if 'btnDebounceCnt' in content and 'btnCnt' in content:
    # Both exist - this is a problem. Use btnCnt consistently.
    content = content.replace('btnDebounceCnt', 'btnCnt')
    print("  Replaced btnDebounceCnt -> btnCnt for consistency")

# ================================================================
# Write the fixed content
# ================================================================
with open(FILE, 'w', encoding='utf-8') as f:
    f.write(content)

print(f"\nDone! File size: {original_len} -> {len(content)} bytes")
print("Please check the file in Arduino IDE and try to compile.")
