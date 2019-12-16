#include <iostream>
#include "ASTNode.h"
#include "RowData.h"
#include "dummy/ColumnVecEvaluator.h"
#include "dummy/DummyEvaluator.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <exception>
#include <memory>

using std::cout;
using std::endl;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::nanoseconds;
using std::unique_ptr;

const int NUM = 10000000;

void verify(RowDataPtr r, int num) {
    for (int i = 0; i < num; i++) {
        double e = (i - 99.88) + (i + 1 + i + 1.1);
        double a = r->getDouble(i, 0);
        if (abs(e - a) > 0.0001) {
            throw new std::runtime_error("wrong answer");
        }
    }
}

RowDataPtr loadData(int num) {
    DummyRowData * const pDummyRaw = new DummyRowData(NUM);
    RowDataPtr pData(pDummyRaw);
    for (int64_t i = 0; i < num; i++) {
        Row row{Datum(i), Datum(i + 1), Datum(i + 1.1)};
        pDummyRaw->addRow(row);
    }
    return pData;
}

void baseline(AstNodePtr root, RowDataPtr pData, int num) {
    DummyEvaluatorBuilder builder;
    unique_ptr<Evaluator<RowDataPtr>> evaluator(builder.build(root, NULL));
    cout << "baseline started..." << endl;
    auto start = high_resolution_clock::now();
    RowDataPtr pRes = evaluator->evaluate(pData);
    auto end = high_resolution_clock::now();
    cout << duration_cast<nanoseconds>(end - start).count() / 1000000000.0 << "s" << endl;

    verify(pRes, num);
}

RowDataPtr loadDataUser(int num) {
    vector<Datum::Type> typeList;
    typeList.push_back(Datum::Int);
    typeList.push_back(Datum::Int);
    typeList.push_back(Datum::Double);

    ColumnData * const pDummyRaw = new ColumnData(NUM, 3, typeList);
    RowDataPtr pData(pDummyRaw);

    for (int64_t i = 0; i < num; i++) {
        Row row{Datum(i), Datum(i + 1), Datum(i + 1.1)};
        pDummyRaw->addRow(row);
    }

    return pData;
}

void userCode(AstNodePtr root, RowDataPtr pData, int num) {

    VecEvaluatorBuilder builder;
    unique_ptr<Evaluator<RowDataPtr>> evaluator(builder.build(root, NULL));
    cout << "column wise execution..." << endl;
    auto start = high_resolution_clock::now();
    RowDataPtr pRes = evaluator->evaluate(pData);
    auto end = high_resolution_clock::now();
    cout << duration_cast<nanoseconds>(end - start).count() / 1000000000.0 << "s" << endl;

    verify(pRes, num);
}

void userCode2(AstNodePtr root, RowDataPtr pData, int num) {

    VecEvaluatorBuilder builder;
    unique_ptr<Evaluator<RowDataPtr>> evaluator(builder.build(root, NULL));
    cout << "vectorize execution..." << endl;
    auto start = high_resolution_clock::now();
    RowDataPtr pRes = evaluator->evaluateVectorize(pData);
    auto end = high_resolution_clock::now();
    cout << duration_cast<nanoseconds>(end - start).count() / 1000000000.0 << "s" << endl;

    verify(pRes, num);
}

void upperlimit(AstNodePtr root, RowDataPtr pData, int num) {
    cout << "upperlimit execution..." << endl;
    auto start = high_resolution_clock::now();

    size_t size = pData->getSize();
    vector<Datum::Type> types;
    types.push_back(Datum::Double);

    ColumnData * pRows = new ColumnData(pData->getSize(), 1, types);
    ColumnData *pColData = dynamic_cast<ColumnData *> (pData.get());

    int64_t *col1 = (int64_t *) pColData->getColumn(0);
    int64_t *col2 = (int64_t *) pColData->getColumn(1);
    double  *col3 = (double *)  pColData->getColumn(2);

    for (size_t i = 0; i < size; i++) {
        double val = col1[i] - 99.98 + col2[i] + col2[i]; 
        pRows->pushSingleColVal(val);
    }

    auto end = high_resolution_clock::now();
    cout << duration_cast<nanoseconds>(end - start).count() / 1000000000.0 << "s" << endl;

    verify(RowDataPtr(pRows), num);
}

void misctests(RowDataPtr pData) {
    cout << "some tests started..." << endl;

    VecEvaluatorBuilder builder;
    AstNodePtr lhs(new Minus(AstNodePtr(new ColumnRef(0)), AstNodePtr(new Constant(99.88))));
    unique_ptr<Evaluator<RowDataPtr>> evaluator(builder.build(lhs, NULL));
    RowDataPtr pRes = evaluator->evaluateVectorize(pData);
    for (int i = 0; i < NUM; ++i) {
        double r = i - 99.88;
        double e = pRes->getDouble(i, 0);
        if (abs(e - r) > 0.0001) {
            throw new std::runtime_error("wrong answer");
        }
    }
    pRes.reset();

    AstNodePtr rhs(new Plus(AstNodePtr(new ColumnRef(1)), AstNodePtr(new ColumnRef(2))));
    unique_ptr<Evaluator<RowDataPtr>> evaluator2(builder.build(rhs, NULL));
    pRes = evaluator2->evaluateVectorize(pData);
    for (int i = 0; i < NUM; ++i) {
        double r = 2*i + 2.1;
        double e = pRes->getDouble(i, 0);
        if (abs(e - r) > 0.0001) {
            throw new std::runtime_error("wrong answer");
        }
    }
    pRes.reset();

    AstNodePtr rhs2(new Plus(AstNodePtr(new ColumnRef(0)), AstNodePtr(new ColumnRef(1))));
    unique_ptr<Evaluator<RowDataPtr>> evaluator3(builder.build(rhs2, NULL));
    pRes = evaluator3->evaluateVectorize(pData);
    for (int i = 0; i < NUM; ++i) {
        int64_t r = 2*i + 1;
        int64_t e = pRes->getInt(i, 0);
        if (r != e) {
            throw new std::runtime_error("wrong answer");
        }
    }

    cout << "some tests end..." << endl;
}

int main() {
    // col(0) = i; col(1) = i + 1; col(2) = i + 1.1
    // lhs = col(0) - 99.98 = i - 99.98
    // rhs = col(1) + col(2) = i + 1 + i + 1.1 = 2i + 2.1
    // expr = 3i + 97.88
    AstNodePtr lhs(new Minus(AstNodePtr(new ColumnRef(0)), AstNodePtr(new Constant(99.88))));
    AstNodePtr rhs(new Plus(AstNodePtr(new ColumnRef(1)), AstNodePtr(new ColumnRef(2))));
    AstNodePtr expr(new Plus(lhs, rhs));

    RowDataPtr pData2 = loadData(NUM);
    baseline(expr, pData2, NUM);

    RowDataPtr pData = loadDataUser(NUM);
    userCode(expr, pData, NUM);
    userCode2(expr, pData, NUM);

    for (int i = 0; i < 1000; i++)
        misctests(pData);

    //hard code performance
    upperlimit(expr, pData, NUM);


    
    return 0;
}