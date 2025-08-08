//
//  tokenizer.cpp
//  分词器实现文件，包含多种分词算法的实现
//
//  Created by MNN on 2023/09/25.
//  ZhaodeWang
//

#include <MNN/MNNDefine.h>
#include "tokenizer.hpp"
#include <fstream>
#include <sstream>
#include <queue>
#include <functional>
#include <random>
#include <codecvt>
#include <regex>
#include <set>
#include <climits>
#include <cctype>
namespace MNN {
namespace Transformer {

// base64
/// Base64编码字符表
static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

/// 判断字符是否为Base64编码字符
/// @param c 待判断的字符
/// @return 如果是Base64字符返回true，否则返回false
static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

/// 计算UTF-8字符的字节长度
/// @param src 指向字符的指针
/// @return 字符的字节长度
static inline size_t one_char_len(const char *src) {
    return "\1\1\1\1\1\1\1\1\1\1\1\1\2\2\3\4"[(*src & 0xFF) >> 4];
}

/// Base64解码函数
/// @param str 待解码的Base64字符串
/// @return 解码后的字符串
static std::string base64_decode(const std::string& str) {
    int in_len = str.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && ( str[in_] != '=') && is_base64(str[in_])) {
        char_array_4[i++] = str[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++) {
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            }
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (i = 0; (i < 3); i++) {
                ret.push_back(char_array_3[i]);
            }
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 4; j++) {
            char_array_4[j] = 0;
        }
        for (j = 0; j < 4; j++) {
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        }
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        for (j = 0; (j < i - 1); j++) {
            ret.push_back(char_array_3[j]);
        }
    }
    return ret;
}

/// 将字符串转换为小写
/// @param str 待转换的字符串
static inline void to_lower_case(std::string& str) {
    for (auto &c : str) {
        if (c >= 'A' && c <= 'Z') {
            c = tolower(static_cast<unsigned char>(c));
        }
    }
}

/// 创建分词器实例
/// @param filename 分词器模型文件路径
/// @return 分词器指针，创建失败返回nullptr
Tokenizer* Tokenizer::createTokenizer(const std::string& filename) {
    Tokenizer* tokenizer = nullptr;
    // 检查文件是否存在
    std::ifstream tok_file(filename);
    if (!tok_file.good()) {
        printf("Failed: can't load tokenzier from: %s.\n", filename.c_str());
        return tokenizer;
    }
    // 检查分词器信息
    std::string line;
    std::getline(tok_file, line);
    std::istringstream line_str(line);
    int magic_number, tokenizer_type;
    line_str >> magic_number;
    if (magic_number != MAGIC_NUMBER) {
        printf("Failed: magic number is wrong from: %s.\n", filename.c_str());
        return tokenizer;
    }
    line_str >> tokenizer_type;
    // 创建对应的分词器实例
    switch (tokenizer_type)
    {
        case SENTENCEPIECE:
            tokenizer = new Sentencepiece();
            break;
        case TIKTOIKEN:
            tokenizer = new Tiktoken();
            break;
        case BERT:
            tokenizer = new BertTokenizer();
            break;
        case HUGGINGFACE:
            tokenizer = new HuggingfaceTokenizer();
            break;
        default:
            return tokenizer;
    }
    // 加载特殊token
    tokenizer->load_special(tok_file);
    // 加载词汇表
    tokenizer->load_vocab(tok_file);
    tok_file.close();
    return tokenizer;
}

/// 判断token是否为停止token
/// @param token 待判断的token ID
/// @return 如果是停止token返回true，否则返回false
bool Tokenizer::is_stop(int token) {
    return std::find(stop_tokens_.begin(), stop_tokens_.end(), token) != stop_tokens_.end();
}

/// 判断token是否为特殊token
/// @param token 待判断的token ID
/// @return 如果是特殊token返回true，否则返回false
bool Tokenizer::is_special(int token) {
    return std::find(special_tokens_.begin(), special_tokens_.end(), token) != special_tokens_.end();
}

/// 加载特殊token
/// @param tok_file 分词器模型文件流
void Tokenizer::load_special(std::ifstream& tok_file) {
    std::string line;
    std::getline(tok_file, line);
    std::istringstream line_str(line);
    int special_num, stop_num, prefix_num;
    line_str >> special_num >> stop_num >> prefix_num;
    std::getline(tok_file, line);
    std::istringstream specail_line(line);
    if (special_num) {
        // 加载特殊token
        special_tokens_.resize(special_num);
        for (int i = 0; i < special_num; i++) {
            specail_line >> special_tokens_[i];
        }
    }
    if (stop_num) {
        // 加载停止token
        stop_tokens_.resize(stop_num);
        for (int i = 0; i < stop_num; i++) {
            specail_line >> stop_tokens_[i];
        }
    }
    if (prefix_num) {
        // 加载前缀token
        prefix_tokens_.resize(prefix_num);
        for (int i = 0; i < prefix_num; i++) {
            specail_line >> prefix_tokens_[i];
        }
    }
}

/// 将文本编码为token ID序列
/// @param str 待编码的文本
/// @return token ID序列
/// @details 该方法首先添加前缀token，然后处理特殊token。如果存在特殊token，
/// 则在文本中查找并替换这些特殊token，其余部分按正常流程编码。
/// 如果没有特殊token，则直接对整个字符串进行编码。
std::vector<int> Tokenizer::encode(const std::string& str) {
    // 初始化结果向量，首先添加前缀token
    std::vector<int> ids = prefix_tokens_;
    
    // 检查是否存在特殊token需要处理
    if (!special_tokens_.empty()) {
        std::string text = str;
        size_t start = 0;  // 记录当前处理位置的起始点
        
        // 遍历文本中的每个字符位置
        for (size_t i = 0; i < text.length(); ++i) {
            // 检查每个特殊token是否在当前位置匹配
            for (auto special_id : special_tokens_) {
                // 获取特殊token对应的字符串表示
                const auto& token = decode(special_id);
                if (token.empty()) continue;  // 跳过空的token
                
                // 检查当前位置是否匹配特殊token
                if (i + token.length() <= text.length() && text.substr(i, token.length()) == token) {
                    // 如果匹配位置之前还有未处理的文本，则先编码这部分文本
                    if (i > start) {
                        encode(text.substr(start, i - start), ids);
                    }
                    
                    // 将匹配到的特殊token ID添加到结果中
                    ids.push_back(special_id);
                    
                    // 更新下一个处理位置的起始点
                    start = i + token.length();
                    i = start - 1;  // 调整循环变量，跳过已处理的部分
                    break;  // 跳出特殊token循环，继续处理下一个字符位置
                }
            }
        }
        
        // 处理最后剩余的文本部分
        if (start < text.length()) {
            encode(text.substr(start), ids);
        }
    } else {
        // 如果没有特殊token，则直接对整个字符串进行编码
        encode(str, ids);
    }
    
    // 返回编码后的token ID序列
    return ids;
}

/// 加载Sentencepiece词汇表
/// @param tok_file 分词器模型文件流
/// @return 加载成功返回true，否则返回false
bool Sentencepiece::load_vocab(std::ifstream& tok_file) {
    std::string line, token;
    std::getline(tok_file, line);
    int vocab_len = std::stoi(line);
    float score;
    int type;
    sentence_pieces_.resize(vocab_len);
    for (int index = 0; index < vocab_len; index++) {
        std::getline(tok_file, line);
        std::istringstream line_str(line);
        line_str >> token >> score >> type;
        token = base64_decode(token);
        auto piece_type = static_cast<PieceType>(type);
        SentencePiece piece = {token, score, piece_type};
        sentence_pieces_[index] = std::move(piece);
        if (piece_type == PieceType::NORMAL) {
            pieces_.insert({token, index});
        } else {
            reserved_id_map_.insert({token, index});
            if (piece_type == PieceType::UNKNOWN) {
                unk_id_ = index;
            }
        }
    }
    return true;
}

/// 根据piece获取对应的ID
/// @param piece 待查找的piece
/// @return 对应的ID，未找到返回未知token ID
int Sentencepiece::piece_to_id(const std::string& piece) const {
    auto it = reserved_id_map_.find(piece);
    if (it != reserved_id_map_.end()) {
        return it->second;
    }
    auto it2 = pieces_.find(piece);
    if (it2 != pieces_.end()) {
        return it2->second;
    }
    return unk_id_;
}

/// 将字节转换为piece表示
/// @param c 待转换的字节
/// @return piece字符串表示
std::string Sentencepiece::byte_to_piece(unsigned char c) const {
    const int len = ::snprintf(nullptr, 0, "<0x%02X>", c);
    std::string s;
    s.resize(len);
    ::snprintf(&s[0], s.size() + 1, "<0x%02X>", c);
    return s;
}

// ref: https://github.com/google/sentencepiece/blob/master/src/bpe_model.cc
/// 使用BPE算法对文本进行编码
/// @param normalized 待编码的标准化文本
/// @param alpha BPE dropout参数，控制合并概率
/// @return 编码结果，包含piece和对应ID的pair向量
Sentencepiece::EncodeResult Sentencepiece::bpe_encode(string_view_ normalized, float alpha) {
    // 工具类开始
    /// 符号对结构体，用于表示两个相邻符号的合并信息
    struct SymbolPair {
        int left;     // 左侧符号索引
        int right;    // 右侧符号索引
        float score;  // 合并得分，越大越好
        size_t size;  // 合并后piece的长度
    };

    /// 符号对比较器，用于优先队列排序
    class SymbolPairComparator {
    public:
        const bool operator()(SymbolPair *h1, SymbolPair *h2) {
            return (h1->score < h2->score || (h1->score == h2->score && h1->left > h2->left));
        }
    };

    /// 符号结构体，表示一个文本符号
    struct Symbol {
        int prev;     // 前一个符号索引，-1表示开始
        int next;     // 后一个符号索引，-1表示结束
        bool freeze = false;  // 是否冻结，冻结的符号不会被合并
        string_view_ piece;   // 符号对应的piece
    };
    // 工具类结束

        // 优先队列类型定义，用于存储待处理的符号对
    using Agenda = std::priority_queue<SymbolPair *, std::vector<SymbolPair *>, SymbolPairComparator>;
    Agenda agenda;  // 优先队列，存储待合并的符号对
    std::vector<Symbol> symbols;  // 符号序列
    symbols.reserve(normalized.size());
    // 反向合并规则映射表，键为合并后的符号，值为原始符号对
    std::unordered_map<string_view_, std::pair<string_view_, string_view_>> rev_merge;
    // 符号对持有者，用于管理符号对的内存
    std::vector<std::unique_ptr<SymbolPair>> symbol_pair_holder;
    // 查找新的符号对并将其插入到优先队列中
    auto MaybeAddNewSymbolPair = [this, &symbol_pair_holder, &symbols, &agenda, &rev_merge](int left, int right) {
        // 如果左或右符号索引无效，或者符号被冻结，则不处理
        if (left == -1 || right == -1 || symbols[left].freeze || symbols[right].freeze) {
            return;
        }
        // 构造合并后的piece
        const string_view_ piece(symbols[left].piece.data(), symbols[left].piece.size() + symbols[right].piece.size());
        std::string piece_str(piece.to_string());
        // 查找合并后的piece是否在词汇表中
        const auto it = pieces_.find(piece_str);
        if (it == pieces_.end()) {
            return;
        }
        // 创建新的符号对并加入优先队列
        symbol_pair_holder.emplace_back(new SymbolPair);
        auto *h = symbol_pair_holder.back().get();
        h->left = left;
        h->right = right;
        h->score = get_score(it->second);  // 获取合并得分
        h->size = piece.size();
        agenda.push(h);

        // 为重新分段构建反向合并规则
        if (is_unused(it->second)) {
            rev_merge[piece] = std::make_pair(symbols[left].piece, symbols[right].piece);
        }
    };
    // 将输入文本分割为字符序列
    int index = 0;
    while (!normalized.empty()) {
        Symbol s;
        // const int mblen = matcher_->PrefixMatch(normalized, &s.freeze);
        // 计算当前字符的字节长度
        int mblen = std::min<int>(normalized.size(), one_char_len(normalized.data()));
        s.piece = string_view_(normalized.data(), mblen);
        s.prev = index == 0 ? -1 : index - 1;  // 设置前一个符号索引
        normalized.remove_prefix(mblen);  // 移动到下一个字符
        s.next = normalized.empty() ? -1 : index + 1;  // 设置后一个符号索引
        ++index;
        symbols.emplace_back(s);
    }

    // 如果符号序列为空，直接返回空结果
    if (symbols.empty()) {
        return {};
    }
        // 查找所有双字符组合
    for (size_t i = 1; i < symbols.size(); ++i) {
        MaybeAddNewSymbolPair(i - 1, i);
    }

    // BPE-dropout: https://arxiv.org/pdf/1910.13267.pdf
    // std::mt19937 *rand_gen = nullptr;
    std::mt19937 rand_gen;
    // 判断是否跳过合并的函数
    auto skip_merge = [&]() {
        if (alpha <= 0.0) return false;  // alpha为0时不跳过
        if (alpha >= 1.0) return true;   // alpha为1时总是跳过
        // if (rand_gen == nullptr) rand_gen = random::GetRandomGenerator();
        std::uniform_real_distribution<> gen(0.0, 1.0);
        return gen(rand_gen) < alpha;    // 根据概率决定是否跳过
    };

    // 主循环：不断合并得分最高的符号对
    while (!agenda.empty()) {
        SymbolPair *top = agenda.top();  // 获取得分最高的符号对
        agenda.pop();

        // 检查符号对是否仍然有效
        if (symbols[top->left].piece.empty() || symbols[top->right].piece.empty() ||
            symbols[top->left].piece.size() + symbols[top->right].piece.size() != top->size) {
            continue;
        }

        // 根据BPE dropout决定是否跳过本次合并
        if (skip_merge()) continue;
        // 使用top规则替换符号
        symbols[top->left].piece = string_view_(
                                                symbols[top->left].piece.data(),
                                                symbols[top->left].piece.size() + symbols[top->right].piece.size());

        // 更新前驱/后继指针
        symbols[top->left].next = symbols[top->right].next;
        if (symbols[top->right].next >= 0) {
            symbols[symbols[top->right].next].prev = top->left;
        }
        symbols[top->right].piece = string_view_("");  // 清空右侧符号的piece

                // 添加符号替换后新产生的符号对
        MaybeAddNewSymbolPair(symbols[top->left].prev, top->left);
        MaybeAddNewSymbolPair(top->left, symbols[top->left].next);
    }

    // 重新分段函数，用于处理未使用的ID
    std::function<void(string_view_, EncodeResult*)> resegment;
    resegment = [this, &resegment, &rev_merge](string_view_ w, EncodeResult *output) -> void {
        std::string w_str(w.to_string());
        const int id = piece_to_id(w_str);
        // std::cout << "piece: " << w << ", id = " << id << std::endl;
        // 如果ID有效且不是未使用的ID，则直接添加到输出
        if (id == -1 || !is_unused(id)) {
            output->emplace_back(w, id);
            return;
        }
        // 查找反向合并规则
        const auto p = rev_merge.find(w);
        if (p == rev_merge.end()) {
            // This block will never be called, as `rev_merge` stores all the
            // resegmentation info for unused id.
            output->emplace_back(w, id);
            return;
        }
        // 递归地重新分段左右两侧的符号
        resegment(p->second.first, output);
        resegment(p->second.second, output);
    };
    EncodeResult output;
    // 遍历所有符号并进行重新分段
    for (int index = 0; index != -1; index = symbols[index].next) {
        resegment(symbols[index].piece, &output);
    }
    return output;
}

/// 对字符串进行编码，将其转换为token ID序列
/// @param str 待编码的字符串
/// @param ids 输出的token ID序列
void Sentencepiece::encode(const std::string& str, std::vector<int>& ids) {
    auto result = bpe_encode(str);
    size_t consumed = 0;
    for (const auto &p : result) {
        const string_view_ w = p.first;   // piece
        const int id = p.second;          // id
        const bool is_unk = (id == unk_id_);
        // 如果是未知token且启用了字节回退机制
        if (is_unk && byte_fall_back_) {
            // 将未知piece分解为UTF-8字节
            for (int i = 0; i < w.size(); ++i) {
                // 创建字节piece
                const char b = w[i];
                const auto piece = byte_to_piece(b);
                auto sp_id = piece_to_id(piece);
                ids.push_back(sp_id);
            }
        } else {
            ids.push_back(id);
        }
    }
}

/// 根据ID解码为对应的piece字符串
/// @param id 待解码的token ID
/// @return 对应的piece字符串
std::string Sentencepiece::decode(int id) {
    auto piece = sentence_pieces_[id].piece;
    int pos = piece.find("▁");
    if (pos != -1) {
        piece.replace(pos, pos + 3, " ");
    }
    return piece;
}

/// 获取指定ID的piece得分
/// @param id piece的ID
/// @return 对应的得分
float Sentencepiece::get_score(int id) const {
    return sentence_pieces_[id].score;
}

/// 判断指定ID的piece是否为未使用类型
/// @param id piece的ID
/// @return 如果是未使用类型返回true，否则返回false
bool Sentencepiece::is_unused(int id) const {
    return sentence_pieces_[id].type == PieceType::UNUSED;
}

/// 判断指定ID的piece是否为控制类型
/// @param id piece的ID
/// @return 如果是控制类型返回true，否则返回false
bool Sentencepiece::is_control(int id) const {
    return sentence_pieces_[id].type == PieceType::CONTROL;
}

/// 加载Tiktoken词汇表
/// @param tok_file 分词器模型文件流
/// @return 加载成功返回true，否则返回false
bool Tiktoken::load_vocab(std::ifstream& tok_file) {
    std::string line;
    std::getline(tok_file, line);
    int vocab_len = std::stoi(line);
    // 加载词汇表
    decoder_.resize(vocab_len);
    for (int i = 0; i < vocab_len; i++) {
        std::getline(tok_file, line);
        auto token = base64_decode(line);
        encoder_.insert({token, i});
        decoder_[i] = token;
    }
    return true;
}

/// 对字符串进行Tiktoken编码
/// @param str 待编码的字符串
/// @param ids 输出的token ID序列
void Tiktoken::encode(const std::string& str, std::vector<int>& ids) {
    if (str.empty()) {
        return;
    }
    auto it = str.begin();
    while(it!=str.end()) {
        auto last_it = it;
        int token_id = encoder_.find(it, str.end());
        if (token_id>=0) { ids.push_back(token_id); }
        else {
            MNN_ERROR("Error: No encoding found for the sequence %s\n", std::string(last_it, it).c_str());
        }
    }
}

/// 根据ID解码为对应的token字符串
/// @param id 待解码的token ID
/// @return 对应的token字符串
std::string Tiktoken::decode(int id) {
    if (id >= decoder_.size()) {
        return "";
    }
    return decoder_[id];
}

/// 加载BERT分词器词汇表
/// @param tok_file 分词器模型文件流
/// @return 加载成功返回true，否则返回false
bool BertTokenizer::load_vocab(std::ifstream& tok_file) {
    std::string line;
    std::getline(tok_file, line);
    int vocab_len = std::stoi(line);
    // 加载词汇表
    decoder_.resize(vocab_len);
    for (int i = 0; i < vocab_len; i++) {
        std::getline(tok_file, line);
        auto token = base64_decode(line);
        encoder_.insert({token, i});
        decoder_[i] = token;
    }
    return true;
}

/// 根据ID解码为对应的token字符串
/// @param id 待解码的token ID
/// @return 对应的token字符串
std::string BertTokenizer::decode(int id) {
    if (id >= decoder_.size()) {
        return "";
    }
    return decoder_[id];
}

/// 使用WordPiece算法对token进行分词
/// @param token 待分词的token
/// @return 分词后的token ID序列
std::vector<int> BertTokenizer::word_piece(const std::string& token) {
    // 首先尝试直接查找完整token
    auto it = encoder_.find(token);
    if (it != encoder_.end()) {
        return {it->second};
    }
    // 如果找不到完整token，则使用WordPiece算法进行子词分词
    std::vector<int> ids;
    std::string current = token;
    while (!current.empty()) {
        int match_id = -1;
        size_t match_pos = 0;
        // 从最长的子串开始匹配
        for (int len = current.size(); len > 0; --len) {
            std::string candidate = current.substr(0, len);
            // 如果不是第一个token，需要添加##前缀
            if (!ids.empty()) {
                candidate = "##" + candidate;
            }
            auto it = encoder_.find(candidate);
            if (it != encoder_.end()) {
                match_id = it->second;
                match_pos = len;
                break;
            }
        }
        // [UNK] 未登录词
        if (match_id == -1) {
            ids.push_back(100);
            break;
        }
        ids.push_back(match_id);
        // 不是第一个词，添加##前缀
        current = current.substr(match_pos);
    }
    return ids;
}

/// 对字符串进行BERT分词器编码
/// @param str 待编码的字符串
/// @param ids 输出的token ID序列
void BertTokenizer::encode(const std::string& str, std::vector<int>& ids) {
    std::vector<std::string> tokens;
    std::string current_token;
    size_t i = 0;
    // 将输入字符串分解为token序列
    while (i < str.size()) {
        current_token.clear();
        unsigned char c = static_cast<unsigned char>(str[i]);
        // 处理多字节UTF-8字符
        if ((c & 0x80) != 0) {
            unsigned char mask = 0xE0; // 1110 0000 for 3-byte char
            if ((c & mask) == mask) {
                current_token = str.substr(i, 3);
                i += 3;
            } else {
                ++i;
                continue;
            }
        }
        // 处理连续的字母和数字序列
        else if (isalnum(c)) {
            while (i < str.size() && isalnum(static_cast<unsigned char>(str[i]))) {
                current_token += tolower(str[i]);
                ++i;
            }
        }
        // 处理标点符号
        else if (ispunct(c)) {
            current_token = str[i];
            ++i;
        }
        // 处理空格、制表符、换行符
        else if (isspace(c)) {
            ++i;
            continue;
        }
        // 处理其他单字节字符
        else {
            current_token = str[i];
            ++i;
        }
        if (!current_token.empty()) {
            tokens.push_back(current_token);
        }
    }

    // 对每个token进行WordPiece分词
    for (auto token : tokens) {
        for (auto id : word_piece(token)) {
            ids.push_back(id);
        }
    }
}

/// 将UTF-8字符串转换为宽字符串
/// @param str UTF-8字符串
/// @return 对应的宽字符串
std::wstring utf8_to_wstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.from_bytes(str);
}

/// 将宽字符串转换为UTF-8字符串
/// @param str 宽字符串
/// @return 对应的UTF-8字符串
std::string wstring_to_utf8(const std::wstring& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
    return myconv.to_bytes(str);
}

/// 将UTF-8字符串的每个字节编码为wchar_t
/// @param token UTF-8字符串
/// @param b2u 字节到宽字符的映射表
/// @param result 输出的宽字符串
void byte_encode_token(const std::string& token,
                       const std::unordered_map<uint8_t, wchar_t>& b2u,
                       std::wstring* result) {
    result->resize(0);
    for (char c : token) {
        wchar_t wc = b2u.at(uint8_t(c));
        result->push_back(wc);
    }
}

/// 加载Huggingface分词器词汇表
/// @param tok_file 分词器模型文件流
/// @return 加载成功返回true，否则返回false
bool HuggingfaceTokenizer::load_vocab(std::ifstream& tok_file) {
    std::string line, token;
    // 获取词汇表长度和合并规则长度
    int vocab_len, merge_len;
    std::getline(tok_file, line);
    std::istringstream line_str(line);
    line_str >> vocab_len >> merge_len;
    // 加载词汇表
    decoder_.resize(vocab_len);
    for (int i = 0; i < vocab_len; i++) {
        std::getline(tok_file, line);
        encoder_.insert({line, i});
        decoder_[i] = line;
    }
    // 加载合并规则
    for (int i = 0; i < merge_len; i++) {
        std::getline(tok_file, line);
        int d = line.find(" ");
        bpe_ranks_.insert({{utf8_to_wstring(line.substr(0, d)),
            utf8_to_wstring(line.substr(d + 1))}, i});
    }
    // 字节到unicode映射
    auto _insert_range = [=](int start, int end) {
        for (int c = start; c <= end; c++) {
            b2u_.insert({uint8_t(c), wchar_t(c)});
        }
    };

    b2u_.clear();
    _insert_range(L'!', L'~');
    _insert_range(L'¡', L'¬');
    _insert_range(L'®', L'ÿ');

    int n = 0;
    for (int b = 0; b < 256; b++) {
        if (b2u_.find(uint8_t(b)) == b2u_.end()) {
            b2u_.insert({uint8_t(b), wchar_t(256 + n)});
            n++;
        }
    }
    for (auto e : b2u_) {
        u2b_.insert({e.second, e.first});
    }
    return true;
}

/// 获取宽字符串中的字符对
/// @param word 输入的宽字符串
/// @param pairs 输出的字符对向量
void get_pairs(const std::wstring& word, std::vector<std::pair<std::wstring, std::wstring>>* pairs) {
    pairs->clear();

    if (word.size() < 2) return;

    wchar_t previous = word[0];
    for (int i = 1; i < word.size(); i++) {
        pairs->push_back({std::wstring(1, previous), std::wstring(1, word[i])});
        previous = word[i];
    }
}

/// 对宽字符串进行BPE分词
/// @param token 输入的宽字符串
/// @param bpe_ranks BPE合并规则排名
/// @param result 输出的分词结果
void HuggingfaceTokenizer::bpe(const std::wstring& token, const BPERanks& bpe_ranks, std::vector<std::wstring>* result) {
    std::set<int> merged;  // 记录已合并的字符对索引
    // 获取左侧未合并的字符对索引
    auto _left = [](int i, std::set<int>& merged) {
        for (int j = i - 1; j >= -1; j--) {
            if (merged.find(j) == merged.end()) return j;
        }
        return -1;
    };
    // 获取右侧未合并的字符对索引
    auto _right = [](int i, int cap, std::set<int>& merged) {
        for (int j = i + 1; j < cap; j++) {
            if (merged.find(j) == merged.end()) return j;
        }
        return cap;
    };

    // 获取字符对
    std::vector<std::pair<std::wstring, std::wstring>> pairs;
    get_pairs(token, &pairs);

    // BPE分词主循环
    while (true) {
        int min_score = INT_MAX;
        int to_merge = -1;  // 待合并的字符对索引

        // 寻找得分最高的可合并字符对
        for (int i = 0; i < pairs.size(); ++i) {
            if (merged.find(i) == merged.end()) {  // 字符对i未被合并
                auto iter = bpe_ranks.find(pairs[i]);
                int score = iter != bpe_ranks.end() ? iter->second : INT_MAX;
                if (score < min_score) {
                    min_score = score;
                    to_merge = i;
                }
            }
        }

        // 如果没有可合并的字符对，则退出循环
        if (to_merge == -1) break;

        // 标记字符对为已合并
        merged.insert(to_merge);
        std::wstring merge_into = pairs[to_merge].first + pairs[to_merge].second;

        // 更新相邻字符对
        int l = _left(to_merge, merged);
        if (l >= 0) pairs[l].second = merge_into;
        int r = _right(to_merge, pairs.size(), merged);
        if (r < pairs.size()) pairs[r].first = merge_into;
    }  // end while (true)

    // 生成最终结果
    if (merged.size() == pairs.size()) {
        result->push_back(token);

    } else {
        for (int i = 0; i < pairs.size(); ++i) {
            if (merged.find(i) == merged.end()) {
                if (_left(i, merged) < 0) result->push_back(pairs[i].first);
                result->push_back(pairs[i].second);
            }
        }
    }
}

/// 对字符串进行Huggingface分词器编码
/// @param str 待编码的字符串
/// @param ids 输出的token ID序列
void HuggingfaceTokenizer::encode(const std::string& str, std::vector<int>& ids) {
    /* original regex from tokenizer.json
        "(?i:'s|'t|'re|'ve|'m|'ll|'d)|[^\\r\\n\\p{L}\\p{N}]?\\p{L}+|\\p{N}| ?[^\\s\\p{L}\\p{N}]+[\\r\\n]*|\\s*[\\r\\n]+|\\s+(?!\\S)|\\s+"
     //    std::regex re("('s|'t|'re|'ve|'m|'ll|'d| ?[[:alpha:]]+| ?[[:digit:]]+| ?[^\\s\\w]+|\\s+)");
     */
    // 使用正则表达式分割文本
    std::regex re("('s|'t|'re|'ve|'m|'ll|'d)|[^\\r\\n[:alpha:][:digit:]]?[[:alpha:]]+|[[:digit:]]| ?[^\\s[:alpha:][:digit:]]+[\r\n]*|\\s*[\\r\\n]+|\\s+(?!\\S)|\\s+", std::regex_constants::icase);
    
    std::string input = str;
    std::vector<std::string> result;
    std::smatch match;
    
    std::string token;
    // 逐个匹配正则表达式
    while (std::regex_search(input, match, re)) {
        token = match.str(0);
        input = match.suffix().str();
        // 将token转换为宽字符串
        std::wstring wtoken;
        for (char c : token) {
            wtoken.push_back(b2u_.at(uint8_t(c)));
        }

        // 对宽字符串进行BPE分词
        std::vector<std::wstring> bpe_tokens;
        bpe(wtoken, bpe_ranks_, &bpe_tokens);

        // 将分词结果转换回UTF-8字符串
        for (auto ws : bpe_tokens) {
            result.push_back(wstring_to_utf8(ws));
        }
    }
    // 将字符串转换为对应的token ID
    for (auto s : result) {
        ids.push_back(encoder_.at(s));
    }
}

/// 根据ID解码为对应的token字符串
/// @param id 待解码的token ID
/// @return 对应的token字符串
std::string HuggingfaceTokenizer::decode(int id) {
    // printf("decode id = %d, %lu, %s#\n", id, decoder_.size(), decoder_.at(id).c_str());
    if (id >= decoder_.size()) {
        return "";
    }
    // 将UTF-8字符串转换为宽字符串
    std::wstring w = utf8_to_wstring(decoder_.at(id));
    std::string r;
    // 将宽字符串转换回字节序列
    for (wchar_t c : w) {
        if (u2b_.find(c) != u2b_.end()) {
            r.push_back(char(u2b_.at(c)));
        }
    }
    return r;
}
}
}
