#ifndef HOMEWORK_CPP_DUMMYEVALUATOR_H
#define HOMEWORK_CPP_DUMMYEVALUATOR_H

#include "../Evaluator.h"
#include "../RowData.h"
#include <vector>
#include <exception>

using std::vector;

typedef vector<Datum> Row;

class DummyRowData : public RowData {
public:
    DummyRowData(int rowCount) {}

    Datum get(int rowID, size_t pos)  {
        return rows[rowID][pos];
    }

    int64_t getInt(int rowID, size_t pos) {
        return rows[rowID][pos].getInt();
    }

    double getDouble(int rowID, size_t pos) {
        return rows[rowID][pos].getDouble();
    }

    vector<Datum::Type>& getSchema() {
        throw std::runtime_error("unsupported operation getSchema");
    }

    size_t getSize() {
        return rows.size();
    }

    void addRow(Row & row) {
        rows.push_back(row);
    }
private:
    vector<Row> rows;
};

class DummyEvalutor : public Evaluator<RowDataPtr> {
public:
    DummyEvalutor(AstNodePtr _root) : root(_root) {}

    RowDataPtr evaluate(RowDataPtr data) {
        DummyRowData * pRows = new DummyRowData(data->getSize());
        size_t size = data->getSize();
        for (size_t i = 0; i < size; i++) {
            Datum val = root->evaluate(data, i);
            Row row;
            row.push_back(val);
            pRows->addRow(row);
        }
        return RowDataPtr(pRows);
    }

private:
    AstNodePtr const root;
};


class DummyEvaluatorBuilder : public EvaluatorBuilder<RowDataPtr> {
public:
    Evaluator<RowDataPtr> * build(AstNodePtr root, void * schema) {
        return new DummyEvalutor(root);
    }
};

#endif //HOMEWORK_CPP_DUMMYEVALUATOR_H
