//
// Created by michiel on 1/1/22.
//

#ifndef LEDS_FORMULA_H
#define LEDS_FORMULA_H

#include "Arduino.h"


enum FormulaType {
  int_formula, double_formula
};

enum FormulaOp {
  op_cond,
  op_eq, op_ge, op_le, op_gt, op_lt,
  op_max, op_min,
  op_plus, op_minus,
  op_times, op_over, op_mod,
  op_power,
  op_abs,
  op_const,
  op_n, op_x, op_t,
  op_none
};

class Form {
public:
  const FormulaOp op;

  explicit Form(FormulaOp op);

  virtual ~Form() = default;

  virtual bool isTimed() const;

  virtual bool isVariable() const;

  virtual int eval(int x, int t) const;

  virtual double eval(double x, double t) const;

  virtual void append(FormulaType type, String &s) const;

  String toString(FormulaType type) const;
};

class VarForm : public Form {
public:
  explicit VarForm(FormulaOp op);

  bool isTimed() const override;

  bool isVariable() const override;

  int eval(int x, int t) const override;

  double eval(double x, double t) const override;

  void append(FormulaType type, String &s) const override;
};

class ConstForm : public Form {
private:
  const int intValue;
  const double doubleValue;

public:
  explicit ConstForm(double value);

  explicit ConstForm(int value);

  int eval(int x, int t) const override;

  double eval(double x, double t) const override;

  void append(FormulaType type, String &s) const override;
};

class UnaryForm : public Form {
protected:
  const Form *a;

public:
  UnaryForm(FormulaOp op, const Form *a);

  ~UnaryForm() override;

  bool isTimed() const override;

  bool isVariable() const override;

  int eval(int x, int t) const override;

  double eval(double x, double t) const override;

  void append(FormulaType type, String &s) const override;
};

class BinaryForm : public UnaryForm {
protected:
  const Form *b;

public:
  BinaryForm(FormulaOp op, const Form *a, const Form *b);

  ~BinaryForm() override;

  bool isTimed() const override;

  bool isVariable() const override;

  int eval(int x, int t) const override;

  double eval(double x, double t) const override;

  void append(FormulaType type, String &s) const override;

  const char *getOperator() const;
};

class TernaryForm : public BinaryForm {
private:
  const Form *c;

public:
  TernaryForm(FormulaOp op, const Form *a, const Form *b, const Form *c);

  ~TernaryForm() override;

  bool isTimed() const override;

  bool isVariable() const override;

  int eval(int x, int t) const override;

  double eval(double x, double t) const override;

  void append(FormulaType type, String &s) const override;
};

Form *parseFormula(const char *formula);


#endif //LEDS_FORMULA_H
