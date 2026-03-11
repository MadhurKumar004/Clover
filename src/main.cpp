#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

#include "lex.h"
#include "parse.h"
#include "sem.h"
#include "codegen.h"

#include <llvm/Support/raw_ostream.h>

static std::string read_file(const char* path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "error: could not open file: " << path << "\n";
        std::exit(1);
    }
    //this if for taking the file as buffer
    std::ostringstream ss;
    // return all the file buffer
    ss << file.rdbuf();
    //return string insdie the stream
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: clover <source-file>\n";
        return 1;
    }
    // 1. Read source
    std::string source = read_file(argv[1]);

    // 2. Lex
    Lex::Lexer lexer(source);
    std::vector<Tok::Token> tokens = lexer.tokenize();

    // 3. Parse
    Parse::Parse parser(std::move(tokens));
    AST::Program program = parser.parse();

    // 4. Semantic analysis
    Sem::SemanticAnalyzer sem(program);
    sem.analyze();
    if (sem.has_errors()) {
        sem.print_errors();
        return 1;
    }

    // 5. Code generation (LLVM IR)
    Codegen::Generator codegen(program);
    auto module = codegen.generate();
    module->print(llvm::outs(), nullptr);

    return 0;
}
