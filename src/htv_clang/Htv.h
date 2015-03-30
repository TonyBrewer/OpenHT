/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#include "clang/Basic/FileManager.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"

#include "clang/Lex/Lexer.h"

#include "clang/AST/AST.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Pragma.h"
#include "clang/Frontend/CompilerInstance.h"

#include "clang/Frontend/ASTConsumers.h"
#include "clang/Tooling/Tooling.h"

#include "HtAssert.h"
