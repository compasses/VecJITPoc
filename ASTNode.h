#ifndef HOMEWORK_CPP_ASTNODE_H
#define HOMEWORK_CPP_ASTNODE_H

#include <algorithm>
#include "RowData.h"

class AstNode;
typedef std::shared_ptr<AstNode> AstNodePtr;

class AstNode {
public:
    virtual ~AstNode() {};
    virtual Datum evaluate(const RowDataPtr r, int rowID) = 0;
    virtual DatumPtr evaluateVectorize(const RowDataPtr data, Datum::Type& resultType, bool& isRef) = 0;

    template<typename L, typename R, typename F>
    void computePlus(L *l, R *r, F *f, ssize_t size) {
        for (int i = 0; i < size; ++i) {
            f[i] = l[i] + r[i];
        }
    }

    template<typename L, typename R, typename F>
    void computeMinus(L *l, R *r, F *f, ssize_t size) {
        for (int i = 0; i < size; ++i) {
            f[i] = l[i] - r[i];
        }
    }

};

class ColumnRef : public AstNode {
public:
    ColumnRef(int _pos) : pos(_pos) {}
    ~ColumnRef() {}

    int getPos() {
        return pos;
    }

    Datum evaluate(const RowDataPtr r, int rowID) {
        return r->get(rowID, pos);
    }

    DatumPtr evaluateVectorize(const RowDataPtr data, Datum::Type& resultType, bool& isRef) {
        DatumPtr ptr = data->getColumn(pos);
        resultType = data->getSchema()[pos];
        isRef = true;
        return ptr;
    }

private:
    const int pos;
};

class Constant : public AstNode {
public:
    Constant(int64_t i) : value(i) { }
    Constant(double d) : value(d) { }
    ~Constant() {}

    Datum evaluate(const RowDataPtr r, int rowID) {
        return value;
    }

    DatumPtr evaluateVectorize(const RowDataPtr data, Datum::Type& resultType, bool& isRef) {
        int size = data->getSize();
        resultType = value.type;
        isRef = false;

        if (value.type == Datum::Double) {
            double *res = new double[size];
            std::fill_n(res, size, value.d);
            return (DatumPtr) res;
        } else {
            int64_t *res = new int64_t[size];
            std::fill_n(res, size, value.i);
            return (DatumPtr) res;
        }
    }

private:
    const Datum value;
};


class Plus : public AstNode {
public:
    Plus(AstNodePtr const l, AstNodePtr const r) : left(l), right(r) { }

    Datum evaluate(const RowDataPtr r, int rowID) {
        Datum lVal = left->evaluate(r, rowID);
        Datum rVal = right->evaluate(r, rowID);
        if (lVal.getType() == Datum::Double || rVal.getType() == Datum::Double) {
            return lVal.getDouble() + rVal.getDouble();
        } else {
            return lVal.getInt() + rVal.getInt();
        }
    }

    DatumPtr evaluateVectorize(const RowDataPtr data, Datum::Type& resultType, bool& isRef) {
        Datum::Type leftType, rightType;
        DatumPtr leftDatums, rightDatums;
        bool leftRef, rightRef;
        DatumPtr result;
        int resultSize = data->getSize();

        leftDatums = left->evaluateVectorize(data, leftType, leftRef);
        rightDatums = right->evaluateVectorize(data, rightType, rightRef);

        if (leftType == Datum::Double || rightType == Datum::Double) {
            result = (DatumPtr) new double[resultSize];
            resultType = Datum::Double;

            if (leftType == Datum::Double && rightType == Datum::Double) {
                computePlus<double, double, double>((double *)leftDatums, (double *)rightDatums, (double *)result, resultSize);
            } else if (leftType == Datum::Double && rightType == Datum::Int) {
                computePlus<double, int64_t, double>((double *)leftDatums, (int64_t *)rightDatums, (double *)result, resultSize);
            } else if (leftType == Datum::Int && rightType == Datum::Double) {
                computePlus<int64_t, double, double>((int64_t *)leftDatums, (double *)rightDatums, (double *)result, resultSize);
            } else {
                throw new std::runtime_error("not supported data type");
            }
        } else {
            result = (DatumPtr) new int64_t[resultSize];
            resultType = Datum::Int;
            computePlus<int64_t, int64_t, int64_t>((int64_t *)leftDatums, (int64_t *)rightDatums, (int64_t *)result, resultSize);
        }
        
        if (!leftRef)
            delete[] leftDatums;
        
        if (!rightRef)
            delete[] rightDatums;
        
        isRef = false;

        return result;
    }

private:
    AstNodePtr const left;
    AstNodePtr const right;
};

class Minus : public AstNode {
public:
    Minus(AstNodePtr const l, AstNodePtr const r) : left(l), right(r) { }

    Datum evaluate(const RowDataPtr r, int rowID) {
        Datum lVal = left->evaluate(r, rowID);
        Datum rVal = right->evaluate(r, rowID);
        if (lVal.getType() == Datum::Double || rVal.getType() == Datum::Double) {
            return lVal.getDouble() - rVal.getDouble();
        } else {
            return lVal.getInt() - rVal.getInt();
        }
    }

    DatumPtr evaluateVectorize(const RowDataPtr data, Datum::Type& resultType, bool& isRef) {
        Datum::Type leftType, rightType;
        DatumPtr leftDatums, rightDatums;
        bool leftRef, rightRef;
        DatumPtr result;
        int resultSize = data->getSize();

        leftDatums = left->evaluateVectorize(data, leftType, leftRef);
        rightDatums = right->evaluateVectorize(data, rightType, rightRef);

        if (leftType == Datum::Double || rightType == Datum::Double) {
            result = (DatumPtr) new double[resultSize];
            resultType = Datum::Double;

            if (leftType == Datum::Double && rightType == Datum::Double) {
                computeMinus<double, double, double>((double *)leftDatums, (double *)rightDatums, (double *)result, resultSize);
            } else if (leftType == Datum::Double && rightType == Datum::Int) {
                computeMinus<double, int64_t, double>((double *)leftDatums, (int64_t *)rightDatums, (double *)result, resultSize);
            } else if (leftType == Datum::Int && rightType == Datum::Double) {
                computeMinus<int64_t, double, double>((int64_t *)leftDatums, (double *)rightDatums, (double *)result, resultSize);
            } else {
                throw new std::runtime_error("not supported data type");
            }
        } else {
            result = (DatumPtr) new int64_t[resultSize];
            resultType = Datum::Int;
            computeMinus<int64_t, int64_t, int64_t>((int64_t *)leftDatums, (int64_t *)rightDatums, (int64_t *)result, resultSize);
        }
        
        if (!leftRef)
            delete[] leftDatums;
        
        if (!rightRef)
            delete[] rightDatums;
        
        isRef = false;

        return result;
    }

private:
    AstNodePtr const left;
    AstNodePtr const right;
};


#endif //HOMEWORK_CPP_ASTNODE_H
