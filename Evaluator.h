#ifndef HOMEWORK_CPP_EVALUATOR_H
#define HOMEWORK_CPP_EVALUATOR_H

#include <cstddef>
#include "RowData.h"
#include "ASTNode.h"

template <typename T>
class Evaluator {
public:
    virtual T evaluate(RowDataPtr data) = 0;
    virtual T evaluateVectorize(RowDataPtr data) {
        throw new std::runtime_error("not supported");
    }

    virtual ~Evaluator() {}
};

template <typename T>
class EvaluatorBuilder {
public:
    virtual Evaluator<T> * build(AstNodePtr root, void * schema) = 0;
    virtual ~EvaluatorBuilder() {}
};

#endif //HOMEWORK_CPP_EVALUATOR_H
