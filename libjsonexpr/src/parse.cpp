#include "jsonexpr/parse.hpp"

#include <iostream>
#include <span>

using namespace jsonexpr;

constexpr bool debug = false;

namespace {
struct token {
    source_location location;
    enum {
        IDENTIFIER,
        OPERATOR,
        NUMBER,
        STRING,
        LITERAL,
        GROUP_OPEN,
        GROUP_CLOSE,
        SEPARATOR,
        DECLARATOR,
        OBJECT_ACCESS,
        ARRAY_OPEN,
        ARRAY_CLOSE,
        OBJECT_OPEN,
        OBJECT_CLOSE
    } type;
};

// Recoverable error: if raised, can try to match something else).
// In contrast, 'error' is unrecoverable and aborts parsing.
struct match_failure : error {};

using parse_error = std::variant<match_failure, error>;

error abort_parse(const token& t, std::string message) {
    return error{
        .position = t.location.position,
        .length   = t.location.content.length(),
        .message  = std::move(message)};
}

error abort_parse(const parse_error& e) {
    if (std::holds_alternative<match_failure>(e)) {
        const auto& f = std::get<match_failure>(e);
        return error{.position = f.position, .length = f.length, .message = f.message};
    } else {
        return std::get<error>(e);
    }
}

error abort_parse(std::string message) {
    return error{.message = std::move(message)};
}

match_failure match_failed(const token& t, std::string message) {
    return match_failure{
        {.position = t.location.position,
         .length   = t.location.content.length(),
         .message  = std::move(message)}};
}

match_failure match_failed(std::string message) {
    return match_failure{{.message = std::move(message)}};
}

bool is_any_of(char c, std::string_view list) noexcept {
    return list.find_first_of(c) != list.npos;
}

bool is_any_of(std::string_view c, const auto&... candidates) noexcept {
    return (false || ... || (c == candidates));
}

bool is_none_of(char c, std::string_view list) noexcept {
    return list.find_first_of(c) == list.npos;
}

const char* view_begin(std::string_view s) noexcept {
    return s.data();
}

const char* view_end(std::string_view s) noexcept {
    return s.data() + s.size();
}

std::size_t
scan_class(std::string_view expression, std::size_t start, std::string_view chars) noexcept {
    for (std::size_t i = start; i < expression.size(); ++i) {
        if (is_none_of(expression[i], chars)) {
            return i;
        }
    }

    return expression.size();
}

std::optional<std::size_t> scan_string(
    std::string_view expression, std::size_t start, char end_char, char escape_char) noexcept {
    bool escaped = false;
    for (std::size_t i = start; i < expression.size(); ++i) {
        if (expression[i] == escape_char && !escaped) {
            escaped = true;
            continue;
        }

        if (expression[i] == end_char && !escaped) {
            return i + 1;
        }

        escaped = false;
    }

    return {};
}

constexpr std::string_view identifier_chars_start = "abcdefghijklmnopqrstuvwxyz_";
constexpr std::string_view identifier_chars       = "abcdefghijklmnopqrstuvwxyz_0123456789";
constexpr std::string_view number_chars_start     = "0123456789";
constexpr std::string_view number_chars           = "0123456789.";
constexpr std::string_view number_exponent_chars  = "eE";
constexpr std::string_view number_sign_chars      = "+-";
constexpr char             group_char_open        = '(';
constexpr char             group_char_close       = ')';
constexpr char             array_char_open        = '[';
constexpr char             array_char_close       = ']';
constexpr char             separator_char         = ',';
constexpr char             access_char            = '.';
constexpr char             object_char_open       = '{';
constexpr char             object_char_close      = '}';
constexpr char             declarator_char        = ':';
constexpr std::string_view operators_chars        = "<>*/%+-^&|=!";
constexpr std::string_view string_chars           = "\"'";
constexpr std::string_view whitespace_chars       = " \t\n\r";
constexpr char             escape_char            = '\\';

expected<std::vector<token>, error> tokenize(std::string_view expression) noexcept {
    std::vector<token> tokens;
    std::size_t        read_pos = 0;

    auto extract_as = [&](std::size_t count, auto type) {
        tokens.push_back(
            {.location = {.position = read_pos, .content = expression.substr(0, count)},
             .type     = type});
        expression = expression.substr(count);
        read_pos += count;
    };

    auto ignore_whitespace = [&] {
        const auto new_pos = expression.find_first_not_of(whitespace_chars);
        if (new_pos == expression.npos) {
            expression = {};
            return;
        }

        expression = expression.substr(new_pos);
        read_pos += new_pos;
    };

    std::string_view last_expression;
    while (!expression.empty()) {
        if (expression == last_expression) {
            return unexpected(error{read_pos, 1, "infinite loop detected in tokenizer (bug)"});
        }

        last_expression = expression;

        ignore_whitespace();
        if (expression.empty()) {
            break;
        }

        const char current_char = expression.front();

        if (current_char == group_char_open) {
            extract_as(1, token::GROUP_OPEN);
        } else if (current_char == group_char_close) {
            extract_as(1, token::GROUP_CLOSE);
        } else if (current_char == array_char_open) {
            extract_as(1, token::ARRAY_OPEN);
        } else if (current_char == array_char_close) {
            extract_as(1, token::ARRAY_CLOSE);
        } else if (current_char == separator_char) {
            extract_as(1, token::SEPARATOR);
        } else if (current_char == object_char_open) {
            extract_as(1, token::OBJECT_OPEN);
        } else if (current_char == object_char_close) {
            extract_as(1, token::OBJECT_CLOSE);
        } else if (current_char == declarator_char) {
            extract_as(1, token::DECLARATOR);
        } else if (current_char == access_char) {
            extract_as(1, token::OBJECT_ACCESS);
        } else if (is_any_of(current_char, operators_chars)) {
            if (expression.size() >= 2u &&
                is_any_of(expression.substr(0, 2), ">=", "<=", "==", "!=", "**", "&&", "||")) {
                extract_as(2, token::OPERATOR);
            } else {
                extract_as(1, token::OPERATOR);
            }
        } else if (is_any_of(current_char, identifier_chars_start)) {
            extract_as(scan_class(expression, 1, identifier_chars), token::IDENTIFIER);
            if (tokens.back().location.content == "and" || tokens.back().location.content == "or" ||
                tokens.back().location.content == "not") {
                tokens.back().type = token::OPERATOR;
            } else if (
                tokens.back().location.content == "true" ||
                tokens.back().location.content == "false" ||
                tokens.back().location.content == "null") {
                tokens.back().type = token::LITERAL;
            }
        } else if (is_any_of(current_char, number_chars_start)) {
            std::size_t end = scan_class(expression, 1, number_chars);
            if (end < expression.size() && is_any_of(expression[end], number_exponent_chars)) {
                if (end + 1 < expression.size() &&
                    is_any_of(expression[end + 1], number_sign_chars)) {
                    ++end;
                }

                end = scan_class(expression, end + 1, number_chars);
            }

            extract_as(end, token::NUMBER);
        } else if (is_any_of(current_char, string_chars)) {
            const auto end = scan_string(expression, 1, current_char, escape_char);
            if (!end.has_value()) {
                return unexpected(error{read_pos, expression.size(), "unterminated string"});
            }

            extract_as(end.value(), token::STRING);
        } else {
            return unexpected(error{read_pos, 1, "unexpected character"});
        }
    }

    return tokens;
}

expected<ast::node, parse_error> try_parse_identifier(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected identifier"));
    }

    token t = tokens.front();
    if (t.type != token::IDENTIFIER) {
        return unexpected(match_failed(t, "expected identifier"));
    }

    tokens = tokens.subspan(1);
    return ast::node{t.location, ast::identifier{t.location.content}};
}

ast::node identifier_to_string(ast::node id) noexcept {
    return ast::node{
        id.location, ast::literal{json(std::string(std::get<ast::identifier>(id.content).name))}};
}

std::string single_to_double_quote(std::string_view input) noexcept {
    std::string str;
    bool        escaped = false;
    for (std::size_t i = 0; i < input.size(); ++i) {
        if (input[i] == escape_char && !escaped) {
            escaped = true;
            continue;
        }

        if (input[i] == '\'') {
            if (!escaped) {
                str += R"(")";
            } else {
                str += R"(')";
            }
        } else if (input[i] == '"') {
            str += R"(\")";
        } else {
            if (escaped) {
                str += '\\';
            }

            str += input[i];
        }

        escaped = false;
    }

    return str;
}

expected<ast::node, parse_error> try_parse_literal(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected literal (number, string, boolean, null)"));
    }

    token t = tokens.front();
    if (t.type != token::NUMBER && t.type != token::STRING && t.type != token::LITERAL) {
        return unexpected(match_failed(t, "expected literal (number, string, boolean, null)"));
    }

    json parsed;
    if (t.type == token::STRING) {
        if (t.location.content[0] == '"') {
            parsed = json::parse(t.location.content, nullptr, false);
        } else {
            // JSON doesn't know about single quote strings; we need to convert the string to
            // an escaped double quote one.
            parsed = json::parse(single_to_double_quote(t.location.content), nullptr, false);
        }
        if (parsed.type() == json::value_t::discarded) {
            return unexpected(abort_parse(t, "could not parse string"));
        }
    } else {
        parsed = json::parse(t.location.content, nullptr, false);
        if (parsed.type() == json::value_t::discarded) {
            return unexpected(abort_parse(t, "could not parse literal"));
        }
    }

    tokens = tokens.subspan(1);
    return ast::node{t.location, ast::literal{std::move(parsed)}};
}

expected<ast::node, parse_error> try_parse_expr(std::span<const token>& tokens) noexcept;

template<typename ElemType, typename ElemParser>
expected<std::vector<ElemType>, parse_error> try_parse_list(
    std::span<const token>& tokens, auto end_token, ElemParser&& try_parse_elem) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected list"));
    }

    std::vector<ElemType> args;
    auto                  args_tokens = tokens;
    std::size_t           prev_size   = 0;
    bool                  first       = true;
    while (!args_tokens.empty()) {
        if (args_tokens.size() == prev_size) {
            return unexpected(
                abort_parse(args_tokens.front(), "infinite loop detected in list parser (bug)"));
        }

        prev_size = args_tokens.size();

        if (args_tokens.front().type == end_token) {
            break;
        }

        if (!first) {
            if (args_tokens.front().type != token::SEPARATOR) {
                return unexpected(abort_parse(args_tokens.front(), "expected ','"));
            }

            args_tokens = args_tokens.subspan(1);
        }

        auto arg = try_parse_elem(args_tokens);
        if (!arg.has_value()) {
            return unexpected(abort_parse(arg.error()));
        }

        args.push_back(std::move(arg.value()));
        first = false;
    }

    tokens = args_tokens;
    return args;
}

expected<ast::node, parse_error> try_parse_function(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected function"));
    }
    if (tokens.size() < 3) {
        return unexpected(match_failed(tokens.front(), "expected function"));
    }
    if (tokens[0].type != token::IDENTIFIER) {
        return unexpected(match_failed(tokens[0], "expected identifier"));
    }
    if (tokens[1].type != token::GROUP_OPEN) {
        return unexpected(match_failed(tokens[1], "expected function argument list"));
    }

    const token& func = tokens.front();

    auto args_tokens = tokens.subspan(2);
    auto args        = try_parse_list<ast::node>(args_tokens, token::GROUP_CLOSE, &try_parse_expr);
    if (!args.has_value()) {
        return unexpected(args.error());
    }

    if (args_tokens.empty()) {
        return unexpected(abort_parse("expected ')'"));
    }

    const token& end_token = args_tokens.front();

    args_tokens = args_tokens.subspan(1);
    tokens      = args_tokens;

    return ast::node{
        {func.location.position,
         std::string_view(view_begin(func.location.content), view_end(end_token.location.content))},
        ast::function{func.location.content, std::move(args.value())}};
}

expected<ast::node, parse_error> try_parse_group(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected group"));
    }
    if (tokens.size() < 2 || tokens.front().type != token::GROUP_OPEN) {
        return unexpected(match_failed(tokens.front(), "expected group"));
    }

    const token& start_token  = tokens.front();
    auto         group_tokens = tokens.subspan(1);
    auto         expr         = try_parse_expr(group_tokens);
    if (!expr.has_value()) {
        return unexpected(abort_parse(expr.error()));
    }

    if (group_tokens.empty()) {
        return unexpected(abort_parse("expected group end"));
    } else if (group_tokens.front().type != token::GROUP_CLOSE) {
        return unexpected(abort_parse(group_tokens.front(), "expected group end"));
    }

    const token& end_token = group_tokens.front();

    group_tokens = group_tokens.subspan(1);
    tokens       = group_tokens;

    expr.value().location.position = start_token.location.position;
    expr.value().location.content  = std::string_view(
        view_begin(start_token.location.content), view_end(end_token.location.content));

    return expr;
}

expected<ast::node, parse_error> try_parse_array(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected array"));
    }
    if (tokens.size() < 2 || tokens.front().type != token::ARRAY_OPEN) {
        return unexpected(match_failed(tokens.front(), "expected array"));
    }

    const token& start_token = tokens.front();
    auto         elem_tokens = tokens.subspan(1);
    auto elems = try_parse_list<ast::node>(elem_tokens, token::ARRAY_CLOSE, &try_parse_expr);
    if (!elems.has_value()) {
        return unexpected(elems.error());
    }

    if (elem_tokens.empty()) {
        return unexpected(abort_parse("expected ']'"));
    }

    const token& end_token = elem_tokens.front();

    elem_tokens = elem_tokens.subspan(1);
    tokens      = elem_tokens;

    return ast::node{
        {start_token.location.position,
         std::string_view(
             view_begin(start_token.location.content), view_end(end_token.location.content))},
        ast::array{std::move(elems.value())}};
}

expected<std::pair<ast::node, ast::node>, parse_error>
try_parse_field(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected field"));
    }

    auto field_tokens = tokens;
    auto key          = try_parse_expr(field_tokens);
    if (!key.has_value()) {
        return unexpected(key.error());
    }

    if (field_tokens.empty()) {
        return unexpected(abort_parse("expected ':'"));
    }
    if (field_tokens.front().type != token::DECLARATOR) {
        return unexpected(abort_parse(field_tokens.front(), "expected ':'"));
    }

    field_tokens = field_tokens.subspan(1);
    auto value   = try_parse_expr(field_tokens);
    if (!value.has_value()) {
        return unexpected(abort_parse(value.error()));
    }

    tokens = field_tokens;
    return std::pair<ast::node, ast::node>{std::move(key.value()), std::move(value.value())};
}

expected<ast::node, parse_error> try_parse_object(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected object"));
    }
    if (tokens.size() < 2 || tokens.front().type != token::OBJECT_OPEN) {
        return unexpected(match_failed(tokens.front(), "expected object"));
    }

    const token& start_token = tokens.front();
    auto         elem_tokens = tokens.subspan(1);
    auto         elems       = try_parse_list<std::pair<ast::node, ast::node>>(
        elem_tokens, token::OBJECT_CLOSE, &try_parse_field);
    if (!elems.has_value()) {
        return unexpected(elems.error());
    }

    if (elem_tokens.empty()) {
        return unexpected(abort_parse("expected '}'"));
    }

    const token& end_token = elem_tokens.front();

    elem_tokens = elem_tokens.subspan(1);
    tokens      = elem_tokens;

    return ast::node{
        {start_token.location.position,
         std::string_view(
             view_begin(start_token.location.content), view_end(end_token.location.content))},
        ast::object{std::move(elems.value())}};
}

bool is_match(const expected<ast::node, parse_error>& result) {
    return result.has_value() || std::holds_alternative<error>(result.error());
}

expected<ast::node, parse_error> try_parse_operand(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected operand"));
    }

    if (auto op = try_parse_group(tokens); is_match(op)) {
        return op;
    }
    if (auto op = try_parse_array(tokens); is_match(op)) {
        return op;
    }
    if (auto op = try_parse_object(tokens); is_match(op)) {
        return op;
    }
    if (auto op = try_parse_function(tokens); is_match(op)) {
        return op;
    }
    if (auto op = try_parse_identifier(tokens); is_match(op)) {
        return op;
    }
    if (auto op = try_parse_literal(tokens); is_match(op)) {
        return op;
    }

    return unexpected(match_failed(tokens.front(), "expected operand"));
}

expected<ast::node, parse_error> try_parse_access(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected array or object access"));
    }

    auto object = try_parse_operand(tokens);
    if (!object.has_value()) {
        return unexpected(abort_parse(object.error()));
    }

    while (!tokens.empty() &&
           (tokens[0].type == token::ARRAY_OPEN || tokens[0].type == token::OBJECT_ACCESS)) {
        const bool array_access = tokens[0].type == token::ARRAY_OPEN;
        tokens                  = tokens.subspan(1);

        expected<ast::node, parse_error> index;
        if (array_access) {
            index = try_parse_expr(tokens);
        } else {
            index = try_parse_identifier(tokens);
        }

        if (!index.has_value()) {
            return unexpected(abort_parse(index.error()));
        }

        const char* end_pos = nullptr;
        if (array_access) {
            if (tokens.empty() || tokens[0].type != token::ARRAY_CLOSE) {
                return unexpected(abort_parse("expected ']'"));
            }

            const token& end_token = tokens.front();
            tokens                 = tokens.subspan(1);
            end_pos                = view_end(end_token.location.content);
        } else {
            index   = identifier_to_string(std::move(index.value()));
            end_pos = view_end(index.value().location.content);
        }

        object = ast::node{
            {object.value().location.position,
             std::string_view(view_begin(object.value().location.content), end_pos)},
            ast::function{"[]", {std::move(object.value()), std::move(index.value())}}};
    }

    return object;
}

expected<ast::node, parse_error> try_parse_unary(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected unary expression"));
    }

    if (tokens.front().type != token::OPERATOR) {
        return try_parse_access(tokens);
    }

    auto         unary_tokens    = tokens;
    const token& parsed_operator = unary_tokens.front();
    unary_tokens                 = unary_tokens.subspan(1);

    auto operand = try_parse_unary(unary_tokens);
    if (!operand.has_value()) {
        return unexpected(abort_parse(operand.error()));
    }

    tokens = unary_tokens;

    return ast::node{
        {parsed_operator.location.position, std::string_view(
                                                view_begin(parsed_operator.location.content),
                                                view_end(operand.value().location.content))},
        ast::function{parsed_operator.location.content, {std::move(operand.value())}}};
}

// From least to highest precedence. Operators with the same precedence are evaluated left-to-right.
const std::vector<std::vector<std::string_view>> operator_precedence = {
    {"||", "or"}, {"&&", "and"},   {"==", "!="}, {"<", "<=", ">", ">="},
    {"+", "-"},   {"*", "/", "%"}, {"^", "**"}};

std::size_t get_precedence(std::string_view op) noexcept {
    for (std::size_t p = 0; p < operator_precedence.size(); ++p) {
        if (std::find(operator_precedence[p].begin(), operator_precedence[p].end(), op) !=
            operator_precedence[p].end()) {
            return p + 1;
        }
    }

    return 0;
}

struct tagged_operator {
    std::string_view token;
    std::size_t      precedence = 0;
};

expected<ast::node, parse_error> try_parse_expr(std::span<const token>& tokens) noexcept {
    if (tokens.empty()) {
        return unexpected(match_failed("expected expression"));
    }

    std::vector<ast::node>       nodes;
    std::vector<tagged_operator> operators;

    std::size_t prev_size  = 0;
    auto        ops_tokens = tokens;
    bool        first      = true;
    while (!ops_tokens.empty()) {
        if (ops_tokens.size() == prev_size) {
            return unexpected(abort_parse(
                ops_tokens.front(), "infinite loop detected in expression parser (bug)"));
        }

        prev_size = ops_tokens.size();

        if (!first) {
            if (ops_tokens.front().type != token::OPERATOR) {
                break;
            }

            const auto op_token = ops_tokens.front().location.content;
            operators.push_back({.token = op_token, .precedence = get_precedence(op_token)});
            ops_tokens = ops_tokens.subspan(1);
        }

        auto operand = try_parse_unary(ops_tokens);
        if (!operand.has_value()) {
            return unexpected(operand.error());
        }

        nodes.push_back(std::move(operand.value()));
        first = false;
    }

    tokens = ops_tokens;

    if (nodes.size() == 1u) {
        return nodes.front();
    }

    for (std::size_t i = 0; i < operators.size();) {
        if (i == operators.size() - 1 || operators[i].precedence >= operators[i + 1].precedence) {
            nodes[i] = ast::node{
                {nodes[i].location.position, std::string_view(
                                                 view_begin(nodes[i].location.content),
                                                 view_end(nodes[i + 1].location.content))},
                ast::function{operators[i].token, {std::move(nodes[i]), std::move(nodes[i + 1])}}};
            nodes.erase(nodes.begin() + (i + 1));
            operators.erase(operators.begin() + i);
            if (i > 0) {
                --i;
            }
        } else {
            ++i;
        }
    }

    return nodes.front();
}
} // namespace

expected<ast::node, error> jsonexpr::parse(std::string_view expression) noexcept {
    const auto tokenizer_result = tokenize(expression);
    if (!tokenizer_result.has_value()) {
        return unexpected(tokenizer_result.error());
    }

    if constexpr (debug) {
        std::cout << "tokens:" << std::endl;
        for (auto token : tokenizer_result.value()) {
            std::cout << token.location.content << std::endl;
        }
        std::cout << std::endl;
    }

    std::span<const token> tokens(tokenizer_result.value().begin(), tokenizer_result.value().end());
    const auto             parse_result = try_parse_expr(tokens);
    if (!parse_result.has_value()) {
        const auto err = parse_result.error();
        return unexpected(std::visit([](const auto& e) -> error { return e; }, err));
    }
    if (!tokens.empty()) {
        return unexpected(abort_parse(tokens.front(), "unexpected content in expression"));
    }

    if constexpr (debug) {
        std::cout << "ast:" << std::endl;
        std::cout << ast::dump(parse_result.value()) << std::endl;
        std::cout << std::endl;
    }

    return parse_result.value();
}
