#!/usr/bin/env python3
import sys

replacements = {
    '＜': '<',
    '＞': '>',
    '（': '(',
    '）': ')',
    '，': ',',
    '＆': '&',
    "> =": ">=",
    "< =": "<=",
    "- >": "->",
    " / ": "/",
    '～': '~'
    # 添加其他需要替换的字符
}

if len(sys.argv) < 2:
    print("Usage: python3 convert.py <filename>")
    sys.exit(1)
    
filename = sys.argv[1]
with open(filename, 'r', encoding='utf-8') as f:
    content = f.read()
    
for ch, repl in replacements.items():
    content = content.replace(ch, repl)
    
with open(filename, 'w', encoding='utf-8') as f:
    f.write(content)