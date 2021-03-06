#include <poincare/hyperbolic_tangent.h>
#include <poincare/hyperbolic_cosine.h>
#include <poincare/hyperbolic_sine.h>
#include <poincare/division.h>
#include <poincare/simplification_engine.h>
#include <poincare/trigonometry.h>
extern "C" {
#include <assert.h>
}
#include <cmath>

namespace Poincare {

Expression::Type HyperbolicTangent::type() const {
  return Type::HyperbolicTangent;
}

Expression * HyperbolicTangent::clone() const {
  HyperbolicTangent * a = new HyperbolicTangent(m_operands, true);
  return a;
}

Expression * HyperbolicTangent::shallowReduce(Context& context, AngleUnit angleUnit) {
  Expression * e = Expression::shallowReduce(context, angleUnit);
  if (e != this) {
    return e;
  }
#if MATRIX_EXACT_REDUCING
  Expression * op = editableOperand(0);
  if (op->type() == Type::Matrix) {
    return SimplificationEngine::map(this, context, angleUnit);
  }
#endif
  return this;
}

template<typename T>
std::complex<T> HyperbolicTangent::computeOnComplex(const std::complex<T> c, AngleUnit angleUnit) {
  return Trigonometry::RoundToMeaningfulDigits(std::tanh(c));
}

}
