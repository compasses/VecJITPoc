#ifndef HOMEWORK_CPP_COLUMNDATA_H
#define HOMEWORK_CPP_COLUMNDATA_H

#include "../Evaluator.h"
#include "../RowData.h"
#include "DummyEvaluator.h"
#include <iostream>

#include <vector>
#include <string>
#include <exception>

using std::vector;

typedef vector<Datum> Row;

// All fixed length type
class ColumnData : public RowData {
public:
    ColumnData(int rowCount, int colCount, vector<Datum::Type>& types) 
        : totalRows(0), typeList(std::move(types)) {
        for (int i = 0; i < colCount; ++i) {
            DatumPtr p;
            switch (typeList[i])
            {
                case Datum::Int:
                    {
                        p = (DatumPtr) new int64_t[rowCount];// malloc (rowCount * sizeof (int64_t));
                        break;
                    }
                case Datum::Double:
                    {
                        p = (DatumPtr) new double[rowCount];// malloc (rowCount * sizeof (double));
                        break;
                    }
                default:
                    throw new std::runtime_error("wrong type");
                    break;
            }
            cols.push_back(p);
        }
    }

    // create with single column value
    ColumnData(int rowCount, Datum::Type type, DatumPtr col) : totalRows(rowCount) {
        releaseBuffer();
        typeList.clear();
        typeList.push_back(type);
        cols.push_back(col);
    } 

    inline Datum get(int rowID, size_t pos)  {
        if (typeList[pos] == Datum::Type::Double) {
            return getDouble(rowID, pos);
        }
        return (int64_t) cols[pos][rowID];
    }

    inline int64_t getInt(int rowID, size_t pos) {
        return (int64_t) cols[pos][rowID];
    }

    inline double getDouble(int rowID, size_t pos) {
        double *c = (double *) cols[pos];
        return c[rowID];
    }

    vector<Datum::Type>& getSchema() {
        return typeList;
    }

    size_t getSize() {
        return totalRows;
    }

    void pushColum(int colId, Datum& d) {
        switch (typeList[colId])
        {
            case Datum::Int:
                {
                    int64_t *inta = cols[colId];
                    inta[totalRows] = d.getInt();
                    break;
                }

            case Datum::Double:
                {
                    double *dou = (double *) cols[colId];
                    dou[totalRows] = d.getDouble();
                    break;
                }
            default:
                throw new std::runtime_error("wrong type");
                break;
        }
    }

    // no null no check
    void addRow(Row & row) {
        for (int i = 0; i < row.size(); ++i) {
            pushColum(i, row[i]);
        }
        totalRows++;
    }

    void pushSingleColVal(Datum val) {
        if (val.type == Datum::Double) {
            double *dou = (double *) cols[0];
            dou[totalRows] = val.getDouble();
        } else {
            int64_t *inta = cols[0];
            inta[totalRows] = val.getInt();
        }
        totalRows++;
    }

    void pushSingleColVal(double val) {
        double *dou = (double *) cols[0];
        dou[totalRows] = val;
        totalRows++;
    }

    void pushSingleColVal(int64_t val) {
        int64_t *inta = (int64_t *) cols[0];
        inta[totalRows] = val;
        totalRows++;
    }

    DatumPtr getColumn(int colId) {
        return cols[colId];
    }

    Datum::Type getColType(int colId) {
        return typeList[colId];
    }

    void releaseBuffer() {
        for (int i = 0; i < cols.size(); ++i)
            delete cols[i];
        cols.clear();
    }

    ~ColumnData() {
        releaseBuffer();
    }

private:
    int totalRows;
    vector<Datum::Type> typeList;
    vector<DatumPtr> cols;
};

// Suppose memory big enough, not do the batch split.
class VecEvalutor : public Evaluator<RowDataPtr> {
public:
    VecEvalutor(AstNodePtr _root) : root(_root) {
    }

    RowDataPtr evaluate(RowDataPtr data) {
        ColumnData *pResult;
        // check result type
        Datum val = root->evaluate(data, 0);
        if (val.type == Datum::Double) {
            vector<Datum::Type> types{Datum::Double};
            pResult = new ColumnData(data->getSize(), 1, types);
        } else {
            vector<Datum::Type> types{Datum::Int};
            pResult = new ColumnData(data->getSize(), 1, types);
        }

        pResult->pushSingleColVal(val);
        DummyRowData * pRows = new DummyRowData(data->getSize());
        size_t size = data->getSize();

        for (size_t i = 1; i < size; i++) {
            Datum val = root->evaluate(data, i);
            pResult->pushSingleColVal(val);
        }
        return RowDataPtr(pResult);
    }

    RowDataPtr evaluateVectorize(RowDataPtr data) {
        ColumnData *pResult;
        Datum::Type resultType;
        bool isRef;

        // check result type, don't care about isRef, since it's a one time test
        DatumPtr val = root->evaluateVectorize(data, resultType, isRef);
        pResult = new ColumnData(data->getSize(), resultType, val);
        return RowDataPtr(pResult);
    }


private:
    AstNodePtr const root;
};


class VecEvaluatorBuilder : public EvaluatorBuilder<RowDataPtr> {
public:
    Evaluator<RowDataPtr> * build(AstNodePtr root, void * schema) {
        return new VecEvalutor(root);
    }
};

#endif //HOMEWORK_CPP_DUMMYEVALUATOR_H
