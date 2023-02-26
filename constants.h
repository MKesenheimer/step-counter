// constants.h
// constant management
// Created by Matthias Kesenheimer, m.kesenheimer@gmx.net

#pragma once

namespace math::constants
{
	/// <summary>
	///
	/// </summary>
	struct Epsilon
	{
		static constexpr double value() { return 0.001; }
	};

	/// <summary>
	///
	/// </summary>
	struct Infinity
	{
		static constexpr double value() { return 9.999e15; }
	};

	/// <summary>
	///
	/// </summary>
	struct EulerGamma
	{
		static constexpr double value() { return 0.5772156649; }
	};

	/// <summary>
	/// Avogadro number
	/// [1/mol]
	/// </summary>
	struct Avogadro
	{
		static constexpr double value() { return 6.023e23; }
	};

	/// <summary>
	///
	/// </summary>
	struct Pi
	{
		static constexpr double value() { return 3.14159265359; }
	};

	/// <summary>
	///
	/// </summary>
	struct InvSqrtTwoPi
	{
		static constexpr double value() { return 0.398942280401433; }
	};

	/// <summary>
	/// 2*sqrt(2* ln(2))
	/// </summary>
	struct TwoSqrtTwoLnTwo
	{
		static constexpr double value() { return 2.35482; }
	};

	/// <summary>
	///
	/// </summary>
	struct Ln10
	{
		static constexpr double value() { return 2.30259; }
	};
}