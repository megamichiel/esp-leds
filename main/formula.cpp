//
// Created by michiel on 1/1/22.
//

#include "formula.h"
#include "includes.h"
#include "util.h"

#define PARSE_LVLS 6

static const int op_lvl[] = {
        0,
        1, 1, 1, 1, 1,
        2, 2,
        3, 3,
        4, 4, 4,
        5,
        6,
        7,
        8, 8, 8
};

static const char *op_sym[] = {
        "?",
        "=", ">=", "<=", ">", "<",
        "max", "min",
        "+", "-",
        "*", "/", "%",
        "^",
        "|",
        "",
        "N",
        "x",
        "t"
};

static const char *findNextOperator(const char *formula, const char *end, FormulaOp op_base, FormulaOp &which) {
  // Scans for the next operator of a certain type

  int lvl = op_lvl[op_base];
  FormulaOp op;
  bool searching = true;
  while (formula != end && searching) {
    for (op = op_base; op_lvl[op] == lvl; op = (FormulaOp) (op + 1)) {
      if (isSubstr(formula, op_sym[op])) {
        searching = false;
        which = op;
        break;
      }
    }
    if (searching) {
      switch (*formula) {
        case '|':
          formula = findNextOperator(formula + 1, end, op_abs, which);
          break;
        case '(':
          formula = findNextOperator(formula + 1, end, op_const, which);
          break;
      }
      ++formula;
    }
  }
  return formula;
}

// Parse a value between operators
static Form *parseFormLiteral(const char *begin, const char *end);

static Form *parseFormula(const char *begin, const char *end, int lvl) {
  while (end != begin && *(end - 1) == ' ')
    --end;

  const char *delim;
  FormulaOp op_base = (FormulaOp) 0, which;

  // For each parse level, find any operators under it
  while (lvl < PARSE_LVLS) {
    while (op_lvl[op_base] != lvl)
      op_base = (FormulaOp) (op_base + 1);

    if ((delim = findNextOperator(begin, end, op_base, which)) != end)
      break;

    ++lvl;
  }

  // If no operator has been found, it must be a literal
  if (lvl >= PARSE_LVLS)
    return parseFormLiteral(begin, end);

  // TODO parse ternary conditional operator properly

  FormulaOp prev;
  Form *tail = nullptr, *form;
  do {
    // First, parse the formula before the first operator
    if ((form = parseFormula(begin, delim, lvl + 1)) == nullptr) {
      delete tail;
      return nullptr;
    }

    tail = tail == nullptr ? form : new BinaryForm(prev, tail, form);
    prev = which;

    if (delim == end || (begin = delim + strlen(op_sym[which])) == end)
      break;

    delim = findNextOperator(begin, end, op_base, which);
  } while (begin != end);

  return tail;
}

static Form *parseFormLiteral(const char *begin, const char *end) {
  // Skip whitespace
  while (begin != end && *begin == ' ')
    ++begin;

  if (begin == end)
    return nullptr;

  // Is it a group?
  if (*begin == '(' && *(end - 1) == ')')
    return parseFormula(begin + 1, end - 1, 0);
  if (*begin == '|' && *(end - 1) == '|')
    return new UnaryForm(op_abs, parseFormula(begin + 1, end - 1, 0));

  // First, is there a constant?
  const char *pos = begin;
  char c;
  while (pos != end && ((c = *pos) >= '0' && c <= '9'))
    ++pos;

  bool isDouble = false;
  if (pos != end && *pos == '.') {
    isDouble = true;
    ++pos;
    while (pos != end && ((c = *pos) >= '0' && c <= '9'))
      ++pos;
  }

  char *copy = substr(begin, pos);
  Form *form, *res = pos == begin ? nullptr
                                  : isDouble ? new ConstForm(atof(copy)) : new ConstForm(atoi(copy));
  delete[] copy;

  // Now check for N, x, t occurrences
  while (pos != end) {
    switch (*pos) {
      case 'N':
      case 'x':
      case 't':
        c = *pos;
        form = new VarForm(c == 'N' ? op_n : c == 'x' ? op_x : op_t);
        res = res == nullptr ? form : new BinaryForm(op_times, res, form);
        break;
      case ' ':
        break;
      default: // Invalid character
        delete res;
        return nullptr;
    }
    ++pos;
  }
  return res;
}

Form *parseFormula(const char *formula) {
  return parseFormula(formula, formula + strlen(formula), 0);
}

Form::Form(FormulaOp op) : op(op) {}

bool Form::isTimed() const {
  return false;
}

bool Form::isVariable() const {
  return false;
}

int Form::eval(int x, int t) const {
  return 0;
}

double Form::eval(double x, double t) const {
  return 0;
}

void Form::append(FormulaType type, String &s) const {}

String Form::toString(FormulaType type) const {
  String s;
  append(type, s);
  return s;
}

VarForm::VarForm(FormulaOp op) : Form(op) {}

bool VarForm::isTimed() const {
  return op == op_t;
}

bool VarForm::isVariable() const {
  return op == op_x;
}

int VarForm::eval(int x, int t) const {
  switch (op) {
    case op_n:
      return NUM_LEDS;
    case op_x:
      return x;
    case op_t:
      return t;
    default:
      return 0;
  }
}

double VarForm::eval(double x, double t) const {
  switch (op) {
    case op_n:
      return NUM_LEDS;
    case op_x:
      return x;
    case op_t:
      return t;
    default:
      return 0;
  }
}

void VarForm::append(FormulaType type, String &s) const {
  s += "Nxt"[op - op_n];
}

ConstForm::ConstForm(double value) : Form(op_const), intValue((int) value), doubleValue(value) {}

ConstForm::ConstForm(int value) : Form(op_const), intValue(value), doubleValue(value) {}

int ConstForm::eval(int x, int t) const {
  return intValue;
}

double ConstForm::eval(double x, double t) const {
  return doubleValue;
}

void ConstForm::append(FormulaType type, String &s) const {
  s += type == int_formula ? String(intValue) : String(doubleValue);
}

UnaryForm::UnaryForm(FormulaOp op, const Form *a) : Form(op), a(a) {}

UnaryForm::~UnaryForm() {
  delete a;
}

bool UnaryForm::isTimed() const {
  return a->isTimed();
}

bool UnaryForm::isVariable() const {
  return a->isVariable();
}

int UnaryForm::eval(int x, int t) const {
  return abs(a->eval(x, t));
}

double UnaryForm::eval(double x, double t) const {
  return abs(a->eval(x, t));
}

void UnaryForm::append(FormulaType type, String &s) const {
  s += "|";
  a->append(type, s);
  s += "|";
}

BinaryForm::BinaryForm(FormulaOp op, const Form *a, const Form *b) : UnaryForm(op, a), b(b) {}

BinaryForm::~BinaryForm() {
  delete b;
}

bool BinaryForm::isTimed() const {
  return a->isTimed() || b->isTimed();
}

bool BinaryForm::isVariable() const {
  return a->isVariable() || b->isVariable();
}

int BinaryForm::eval(int x, int t) const {
  switch (op) {
    case op_eq:
      return a->eval(x, t) == b->eval(x, t) ? 1 : 0;
    case op_ge:
      return a->eval(x, t) >= b->eval(x, t) ? 1 : 0;
    case op_le:
      return a->eval(x, t) <= b->eval(x, t) ? 1 : 0;
    case op_gt:
      return a->eval(x, t) > b->eval(x, t) ? 1 : 0;
    case op_lt:
      return a->eval(x, t) < b->eval(x, t) ? 1 : 0;
    case op_max:
      return max(a->eval(x, t), b->eval(x, t));
    case op_min:
      return min(a->eval(x, t), b->eval(x, t));
    case op_plus:
      return a->eval(x, t) + b->eval(x, t);
    case op_minus:
      return a->eval(x, t) - b->eval(x, t);
    case op_times:
      return a->eval(x, t) * b->eval(x, t);
    case op_over:
      return a->eval(x, t) / b->eval(x, t);
    case op_mod:
      return a->eval(x, t) % b->eval(x, t);
    case op_power: {
      int v = a->eval(x, t), pow = b->eval(x, t), out = 1;
      while (--pow >= 0)
        out *= v;
      return out;
    }
    default:
      return 0;
  }
}

double BinaryForm::eval(double x, double t) const {
  switch (op) {
    case op_eq:
      return a->eval(x, t) == b->eval(x, t) ? 1 : 0;
    case op_ge:
      return a->eval(x, t) >= b->eval(x, t) ? 1 : 0;
    case op_le:
      return a->eval(x, t) <= b->eval(x, t) ? 1 : 0;
    case op_gt:
      return a->eval(x, t) > b->eval(x, t) ? 1 : 0;
    case op_lt:
      return a->eval(x, t) < b->eval(x, t) ? 1 : 0;
    case op_max:
      return max(a->eval(x, t), b->eval(x, t));
    case op_min:
      return min(a->eval(x, t), b->eval(x, t));
    case op_plus:
      return a->eval(x, t) + b->eval(x, t);
    case op_minus:
      return a->eval(x, t) - b->eval(x, t);
    case op_times:
      return a->eval(x, t) * b->eval(x, t);
    case op_over:
      return a->eval(x, t) / b->eval(x, t);
    case op_mod:
      return remainder(a->eval(x, t), b->eval(x, t));
    case op_power: {
      double v = a->eval(x, t), pow = b->eval(x, t), out = 1;
      while (--pow >= 0)
        out *= v;
      return out;
    }
    default:
      return 0;
  }
}

void BinaryForm::append(FormulaType type, String &s) const {
  if (op_lvl[a->op] < op)
    s += "(";
  a->append(type, s);
  if (op_lvl[a->op] < op)
    s += ")";
  s += " ";
  s += getOperator();
  s += " ";

  if (op_lvl[b->op] > op)
    s += "(";
  b->append(type, s);
  if (op_lvl[b->op] > op)
    s += ")";
}

const char *BinaryForm::getOperator() const {
  switch (op) {
    case op_eq:
      return "=";
    case op_ge:
      return ">=";
    case op_le:
      return "<=";
    case op_gt:
      return ">";
    case op_lt:
      return "<";
    case op_max:
      return "max";
    case op_min:
      return "min";
    case op_plus:
      return "+";
    case op_minus:
      return "-";
    case op_times:
      return "*";
    case op_over:
      return "/";
    case op_mod:
      return "%";
    case op_power:
      return "^";
    default:
      return "";
  }
}

TernaryForm::TernaryForm(FormulaOp op, const Form *a, const Form *b, const Form *c) : BinaryForm(op, a, b), c(c) {}

TernaryForm::~TernaryForm() {
  delete c;
}

bool TernaryForm::isTimed() const {
  return a->isTimed() || b->isTimed() || c->isTimed();
}

bool TernaryForm::isVariable() const {
  return a->isVariable() || b->isVariable() || c->isVariable();
}

int TernaryForm::eval(int x, int t) const {
  return (a->eval(x, t) != 0 ? b : c)->eval(x, t);
}

double TernaryForm::eval(double x, double t) const {
  return (a->eval(x, t) != 0 ? b : c)->eval(x, t);
}

void TernaryForm::append(FormulaType type, String &s) const {
  a->append(type, s);
  s += " ? ";
  b->append(type, s);
  s += " : ";
  c->append(type, s);
}
