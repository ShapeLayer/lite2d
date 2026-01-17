#ifndef __LITE2D_EXPRESSION_H__
#pragma once
#define __LITE2D_EXPRESSION_H__

#include <string>
#include <vector>

/**
 * Blend mode for expression parameter application.
 * - Additive: parameter delta is added to current value.
 * - Override: parameter delta overrides current value based on priority.
 */
enum class BlendMode
{
  Additive,
  Override
};

/**
 * Represents a single parameter change in an expression.
 * @param param_id The ID of the parameter to modify.
 * @param delta The change in value to apply.
 * @param mode The blend mode (additive or override).
 * @param priority The priority for override mode.
 */
struct ExpressionParameter
{
  std::string param_id;
  float delta { 0 };
  BlendMode mode{ BlendMode::Additive };
  int priority { 0 };  // for override
};

/**
 * Represents an expression consisting of multiple parameter changes.
 * @param name The name of the expression.
 * @param params The list of parameter changes in the expression.
 */
struct Expression
{
  std::string name;
  std::vector<ExpressionParameter> params;
};

#endif  // __LITE2D_EXPRESSION_H__
