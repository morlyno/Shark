#include "skpch.h"
#include "Math.h"

#include <glm/gtx/euler_angles.hpp>

namespace Shark::Math {

	bool DecomposeTransform(const glm::mat4& ModelMatrix, glm::vec3& out_Translation, glm::vec3& out_Euler, glm::vec3& out_Scale)
	{
		using namespace glm;

		mat4 LocalMatrix(ModelMatrix);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<float>()))
			return false;

#if 1
		// perspectiveMatrix is used to solve for perspective, but it also provides
		// an easy way to test for singularity of the upper 3x3 component.
		mat4 PerspectiveMatrix(LocalMatrix);

		for (length_t i = 0; i < 3; i++)
			PerspectiveMatrix[i][3] = static_cast<float>(0);
		PerspectiveMatrix[3][3] = static_cast<float>(1);

		/// TODO: Fixme!
		if (epsilonEqual(determinant(PerspectiveMatrix), static_cast<float>(0), epsilon<float>()))
			return false;

#endif

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<float>(0), epsilon<float>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<float>(0), epsilon<float>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<float>(0), epsilon<float>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<float>(0);
			LocalMatrix[3][3] = static_cast<float>(1);
		}

		// Next take care of translation (easy).
		out_Translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

#if 0
		vec3 Row[3];

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		out_Scale.x = length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<float>(1));
		out_Scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<float>(1));
		out_Scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<float>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
		// Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		// if (dot(Row[0], Pdum3) < 0)
		// {
		// 	for (length_t i = 0; i < 3; i++)
		// 	{
		// 		Scale[i] *= static_cast<float>(-1);
		// 		Row[i] *= static_cast<float>(-1);
		// 	}
		// }

#else
		vec3 Row[3], Pdum3, Skew;

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		out_Scale.x = length(Row[0]);// v3Length(Row[0]);

		Row[0] = detail::scale(Row[0], static_cast<float>(1));

		// Compute XY shear factor and make 2nd row orthogonal to 1st.
		Skew.z = dot(Row[0], Row[1]);
		Row[1] = detail::combine(Row[1], Row[0], static_cast<float>(1), -Skew.z);

		// Now, compute Y scale and normalize 2nd row.
		out_Scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<float>(1));
		Skew.z /= out_Scale.y;

		// Compute XZ and YZ shears, orthogonalize 3rd row.
		Skew.y = glm::dot(Row[0], Row[2]);
		Row[2] = detail::combine(Row[2], Row[0], static_cast<float>(1), -Skew.y);
		Skew.x = glm::dot(Row[1], Row[2]);
		Row[2] = detail::combine(Row[2], Row[1], static_cast<float>(1), -Skew.x);

		// Next, get Z scale and normalize 3rd row.
		out_Scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<float>(1));
		Skew.y /= out_Scale.z;
		Skew.x /= out_Scale.z;

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				out_Scale[i] *= static_cast<float>(-1);
				Row[i] *= static_cast<float>(-1);
			}
		}
#endif

		// Now, get the rotations out, as described in the gem.

		// FIXME - Add the ability to return either quaternions (which are
		// easier to recompose with) or Euler angles (rx, ry, rz), which
		// are easier for authors to deal with. The latter will only be useful
		// when we fix https://bugs.webkit.org/show_bug.cgi?id=23799, so I
		// will leave the Euler angle code here for now.

		out_Euler.y = asin(-Row[0][2]);
		if (cos(out_Euler.y) != 0) {
		    out_Euler.x = atan2(Row[1][2], Row[2][2]);
		    out_Euler.z = atan2(Row[0][1], Row[0][0]);
		} else {
		    out_Euler.x = atan2(-Row[2][0], Row[1][1]);
		    out_Euler.z = 0;
		}

		return true;
	}

	bool DecomposeTranslation(const glm::mat4& ModelMatrix, glm::vec3& out_Translation)
	{
		using namespace glm;

		mat4 LocalMatrix(ModelMatrix);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<float>()))
			return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<float>(0), epsilon<float>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<float>(0), epsilon<float>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<float>(0), epsilon<float>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<float>(0);
			LocalMatrix[3][3] = static_cast<float>(1);
		}

		// Next take care of translation (easy).
		out_Translation = vec3(LocalMatrix[3]);
		return true;
	}

}

