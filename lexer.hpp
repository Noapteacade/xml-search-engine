#pragma once
#include <string>
#include <string_view>
#include <cctype>
#include <cstddef>

class Lexer {
    std::string_view content;
    
    void trim_left() {
        while (!content.empty() && std::isspace(static_cast<unsigned char>(content.front()))) {
            content.remove_prefix(1);
        }
    }
    
    std::string chop_left(size_t n) {
        std::string token(content.substr(0, n));
        content.remove_prefix(n);
        return token;
    }
    
    template<typename Predicate>
    std::string chop_while(Predicate pred, bool include_last = false) {
        size_t n = 0;
        while (n < content.size() && pred(content[n])) {
            n++;
        }
        if (include_last && n < content.size()) n++;
        return chop_left(n);
    }
    
    // 返回 UTF-8 字符的字节长度
    static size_t utf8_char_len(unsigned char c) {
        if ((c & 0x80) == 0) return 1;           // ASCII
        if ((c & 0xE0) == 0xC0) return 2;        // 2 字节
        if ((c & 0xF0) == 0xE0) return 3;        // 3 字节（中文）
        if ((c & 0xF8) == 0xF0) return 4;        // 4 字节（emoji）
        return 1; // 无效 UTF-8，回退
    }
    
public:
    Lexer(const std::string& str) : content(str) {}
    
    std::string next_token() {
        trim_left();
        if (content.empty()) return "";
        
        unsigned char first = static_cast<unsigned char>(content.front());
        
        // 特殊标记 {$...}
        if (content.size() >= 2 && content[0] == '{' && content[1] == '$') {
            return chop_while([](char c) { return c != '}'; }, true);
        }
        
        // 数字（包括小数点）
        if (std::isdigit(first)) {
            return chop_while([](char c) { return std::isdigit(c) || c == '.'; });
        }
        
        // ASCII 字母或下划线
        if (std::isalpha(first)) {
            return chop_while([](char c) { return std::isalnum(c) || c == '_'; });
        }
        
        // UTF-8 多字节字符（中文、日文、emoji 等）
        if ((first & 0x80) != 0) {
            size_t len = utf8_char_len(first);
            if (len > 1 && content.size() >= len) {
                return chop_left(len);
            }
            // 无效 UTF-8，跳过这一个字节
            return chop_left(1);
        }
        
        // 其他单个字符（标点、运算符等）
        return chop_left(1);
    }
};