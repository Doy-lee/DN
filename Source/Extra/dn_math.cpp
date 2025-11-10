#define DN_MATH_CPP

#if defined(_CLANGD)
  #include "dn_math.h"
#endif

DN_API bool operator==(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y);
  return result;
}

DN_API bool operator!=(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator>=(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y);
  return result;
}

DN_API bool operator<=(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y);
  return result;
}

DN_API bool operator<(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y);
  return result;
}

DN_API bool operator>(DN_V2I32 lhs, DN_V2I32 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y);
  return result;
}

DN_API DN_V2I32 operator-(DN_V2I32 lhs, DN_V2I32 rhs)
{
  DN_V2I32 result = DN_V2I32_From2N(lhs.x - rhs.x, lhs.y - rhs.y);
  return result;
}

DN_API DN_V2I32 operator-(DN_V2I32 lhs)
{
  DN_V2I32 result = DN_V2I32_From2N(-lhs.x, -lhs.y);
  return result;
}

DN_API DN_V2I32 operator+(DN_V2I32 lhs, DN_V2I32 rhs)
{
  DN_V2I32 result = DN_V2I32_From2N(lhs.x + rhs.x, lhs.y + rhs.y);
  return result;
}

DN_API DN_V2I32 operator*(DN_V2I32 lhs, DN_V2I32 rhs)
{
  DN_V2I32 result = DN_V2I32_From2N(lhs.x * rhs.x, lhs.y * rhs.y);
  return result;
}

DN_API DN_V2I32 operator*(DN_V2I32 lhs, DN_F32 rhs)
{
  DN_V2I32 result = DN_V2I32_From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2I32 operator*(DN_V2I32 lhs, int32_t rhs)
{
  DN_V2I32 result = DN_V2I32_From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2I32 operator/(DN_V2I32 lhs, DN_V2I32 rhs)
{
  DN_V2I32 result = DN_V2I32_From2N(lhs.x / rhs.x, lhs.y / rhs.y);
  return result;
}

DN_API DN_V2I32 operator/(DN_V2I32 lhs, DN_F32 rhs)
{
  DN_V2I32 result = DN_V2I32_From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2I32 operator/(DN_V2I32 lhs, int32_t rhs)
{
  DN_V2I32 result = DN_V2I32_From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2I32 &operator*=(DN_V2I32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2I32 &operator*=(DN_V2I32 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2I32 &operator*=(DN_V2I32 &lhs, int32_t rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2I32 &operator/=(DN_V2I32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2I32 &operator/=(DN_V2I32 &lhs, DN_F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2I32 &operator/=(DN_V2I32 &lhs, int32_t rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2I32 &operator-=(DN_V2I32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2I32 &operator+=(DN_V2I32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2I32 DN_V2I32_Min(DN_V2I32 a, DN_V2I32 b)
{
  DN_V2I32 result = DN_V2I32_From2N(DN_Min(a.x, b.x), DN_Min(a.y, b.y));
  return result;
}

DN_API DN_V2I32 DN_V2I32_Max(DN_V2I32 a, DN_V2I32 b)
{
  DN_V2I32 result = DN_V2I32_From2N(DN_Max(a.x, b.x), DN_Max(a.y, b.y));
  return result;
}

DN_API DN_V2I32 DN_V2I32_Abs(DN_V2I32 a)
{
  DN_V2I32 result = DN_V2I32_From2N(DN_Abs(a.x), DN_Abs(a.y));
  return result;
}

DN_API bool operator!=(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator==(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y);
  return result;
}

DN_API bool operator>=(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y);
  return result;
}

DN_API bool operator<=(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y);
  return result;
}

DN_API bool operator<(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y);
  return result;
}

DN_API bool operator>(DN_V2U16 lhs, DN_V2U16 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y);
  return result;
}

DN_API DN_V2U16 operator-(DN_V2U16 lhs, DN_V2U16 rhs)
{
  DN_V2U16 result = DN_V2U16_From2N(lhs.x - rhs.x, lhs.y - rhs.y);
  return result;
}

DN_API DN_V2U16 operator+(DN_V2U16 lhs, DN_V2U16 rhs)
{
  DN_V2U16 result = DN_V2U16_From2N(lhs.x + rhs.x, lhs.y + rhs.y);
  return result;
}

DN_API DN_V2U16 operator*(DN_V2U16 lhs, DN_V2U16 rhs)
{
  DN_V2U16 result = DN_V2U16_From2N(lhs.x * rhs.x, lhs.y * rhs.y);
  return result;
}

DN_API DN_V2U16 operator*(DN_V2U16 lhs, DN_F32 rhs)
{
  DN_V2U16 result = DN_V2U16_From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2U16 operator*(DN_V2U16 lhs, int32_t rhs)
{
  DN_V2U16 result = DN_V2U16_From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2U16 operator/(DN_V2U16 lhs, DN_V2U16 rhs)
{
  DN_V2U16 result = DN_V2U16_From2N(lhs.x / rhs.x, lhs.y / rhs.y);
  return result;
}

DN_API DN_V2U16 operator/(DN_V2U16 lhs, DN_F32 rhs)
{
  DN_V2U16 result = DN_V2U16_From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2U16 operator/(DN_V2U16 lhs, int32_t rhs)
{
  DN_V2U16 result = DN_V2U16_From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2U16 &operator*=(DN_V2U16 &lhs, DN_V2U16 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2U16 &operator*=(DN_V2U16 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2U16 &operator*=(DN_V2U16 &lhs, int32_t rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2U16 &operator/=(DN_V2U16 &lhs, DN_V2U16 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2U16 &operator/=(DN_V2U16 &lhs, DN_F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2U16 &operator/=(DN_V2U16 &lhs, int32_t rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2U16 &operator-=(DN_V2U16 &lhs, DN_V2U16 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2U16 &operator+=(DN_V2U16 &lhs, DN_V2U16 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API bool operator!=(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator==(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y);
  return result;
}

DN_API bool operator>=(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y);
  return result;
}

DN_API bool operator<=(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y);
  return result;
}

DN_API bool operator<(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y);
  return result;
}

DN_API bool operator>(DN_V2F32 lhs, DN_V2F32 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs)
{
  DN_V2F32 result = DN_V2F32_From2N(-lhs.x, -lhs.y);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x - rhs.x, lhs.y - rhs.y);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs, DN_V2I32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x - rhs.x, lhs.y - rhs.y);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs, DN_F32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x - rhs, lhs.y - rhs);
  return result;
}

DN_API DN_V2F32 operator-(DN_V2F32 lhs, int32_t rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x - rhs, lhs.y - rhs);
  return result;
}

DN_API DN_V2F32 operator+(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x + rhs.x, lhs.y + rhs.y);
  return result;
}

DN_API DN_V2F32 operator+(DN_V2F32 lhs, DN_V2I32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x + rhs.x, lhs.y + rhs.y);
  return result;
}

DN_API DN_V2F32 operator+(DN_V2F32 lhs, DN_F32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x + rhs, lhs.y + rhs);
  return result;
}

DN_API DN_V2F32 operator+(DN_V2F32 lhs, int32_t rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x + rhs, lhs.y + rhs);
  return result;
}

DN_API DN_V2F32 operator*(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x * rhs.x, lhs.y * rhs.y);
  return result;
}

DN_API DN_V2F32 operator*(DN_V2F32 lhs, DN_V2I32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x * rhs.x, lhs.y * rhs.y);
  return result;
}

DN_API DN_V2F32 operator*(DN_V2F32 lhs, DN_F32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2F32 operator*(DN_V2F32 lhs, int32_t rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x * rhs, lhs.y * rhs);
  return result;
}

DN_API DN_V2F32 operator/(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x / rhs.x, lhs.y / rhs.y);
  return result;
}

DN_API DN_V2F32 operator/(DN_V2F32 lhs, DN_V2I32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x / rhs.x, lhs.y / rhs.y);
  return result;
}

DN_API DN_V2F32 operator/(DN_V2F32 lhs, DN_F32 rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2F32 operator/(DN_V2F32 lhs, int32_t rhs)
{
  DN_V2F32 result = DN_V2F32_From2N(lhs.x / rhs, lhs.y / rhs);
  return result;
}

DN_API DN_V2F32 &operator*=(DN_V2F32 &lhs, DN_V2F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2F32 &operator*=(DN_V2F32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2F32 &operator*=(DN_V2F32 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2F32 &operator*=(DN_V2F32 &lhs, int32_t rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V2F32 &operator/=(DN_V2F32 &lhs, DN_V2F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2F32 &operator/=(DN_V2F32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2F32 &operator/=(DN_V2F32 &lhs, DN_F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2F32 &operator/=(DN_V2F32 &lhs, int32_t rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V2F32 &operator-=(DN_V2F32 &lhs, DN_V2F32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2F32 &operator-=(DN_V2F32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2F32 &operator-=(DN_V2F32 &lhs, DN_F32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2F32 &operator-=(DN_V2F32 &lhs, int32_t rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V2F32 &operator+=(DN_V2F32 &lhs, DN_V2F32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2F32 &operator+=(DN_V2F32 &lhs, DN_V2I32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2F32 &operator+=(DN_V2F32 &lhs, DN_F32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2F32 &operator+=(DN_V2F32 &lhs, int32_t rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_V2F32 DN_V2F32_Min(DN_V2F32 a, DN_V2F32 b)
{
  DN_V2F32 result = DN_V2F32_From2N(DN_Min(a.x, b.x), DN_Min(a.y, b.y));
  return result;
}

DN_API DN_V2F32 DN_V2F32_Max(DN_V2F32 a, DN_V2F32 b)
{
  DN_V2F32 result = DN_V2F32_From2N(DN_Max(a.x, b.x), DN_Max(a.y, b.y));
  return result;
}

DN_API DN_V2F32 DN_V2F32_Abs(DN_V2F32 a)
{
  DN_V2F32 result = DN_V2F32_From2N(DN_Abs(a.x), DN_Abs(a.y));
  return result;
}

DN_API DN_F32 DN_V2F32_Dot(DN_V2F32 a, DN_V2F32 b)
{
  // NOTE: Scalar projection of B onto A /////////////////////////////////////////////////////////
  //
  // Scalar projection calculates the signed distance between `b` and `a`
  // where `a` is a unit vector then, the dot product calculates the projection
  // of `b` onto the infinite line that the direction of `a` represents. This
  // calculation is the signed distance.
  //
  // signed_distance = dot_product(a, b) = (a.x * b.x) + (a.y * b.y)
  //
  // Y
  // ^      b
  // |     /|
  // |    / |
  // |   /  |
  // |  /   | Projection
  // | /    |
  // |/     V
  // +--->--------> X
  // .   a  .
  // .      .
  // |------| <- Calculated signed distance
  //
  // The signed-ness of the result indicates the relationship:
  //
  // Distance <0  means `b` is behind           `a`
  // Distance >0  means `b` is in-front of      `a`
  // Distance ==0 means `b` is perpendicular to `a`
  //
  // If `a` is not normalized then the signed-ness of the result still holds
  // however result no longer represents the actual distance between the
  // 2 objects. One of the vectors must be normalised (e.g. turned into a unit
  // vector).
  //
  // NOTE: DN_V projection /////////////////////////////////////////////////////////////////////
  //
  // DN_V projection calculates the exact X,Y coordinates of where `b` meets
  // `a` when it was projected. This is calculated by multipying the
  // 'scalar projection' result by the unit vector of `a`
  //
  // vector_projection = a * signed_distance = a * dot_product(a, b)

  DN_F32 result = (a.x * b.x) + (a.y * b.y);
  return result;
}

DN_API DN_F32 DN_V2F32_LengthSq2V2(DN_V2F32 lhs, DN_V2F32 rhs)
{
  // NOTE: Pythagoras's theorem (a^2 + b^2 = c^2) without the square root
  DN_F32 a         = rhs.x - lhs.x;
  DN_F32 b         = rhs.y - lhs.y;
  DN_F32 c_squared = DN_Squared(a) + DN_Squared(b);
  DN_F32 result    = c_squared;
  return result;
}

DN_API bool DN_V2F32_LengthSqIsWithin2V2(DN_V2F32 lhs, DN_V2F32 rhs, DN_F32 within_amount_sq)
{
  DN_F32 dist   = DN_V2F32_LengthSq2V2(lhs, rhs);
  bool   result = dist <= within_amount_sq;
  return result;
}

DN_API DN_F32 DN_V2F32_Length2V2(DN_V2F32 lhs, DN_V2F32 rhs)
{
  DN_F32 result_squared = DN_V2F32_LengthSq2V2(lhs, rhs);
  DN_F32 result         = DN_SqrtF32(result_squared);
  return result;
}

DN_API DN_F32 DN_V2F32_LengthSq(DN_V2F32 lhs)
{
  // NOTE: Pythagoras's theorem without the square root
  DN_F32 c_squared = DN_Squared(lhs.x) + DN_Squared(lhs.y);
  DN_F32 result    = c_squared;
  return result;
}

DN_API DN_F32 DN_V2F32_Length(DN_V2F32 lhs)
{
  DN_F32 c_squared = DN_V2F32_LengthSq(lhs);
  DN_F32 result    = DN_SqrtF32(c_squared);
  return result;
}

DN_API DN_V2F32 DN_V2F32_Normalise(DN_V2F32 a)
{
  DN_F32   length = DN_V2F32_Length(a);
  DN_V2F32 result = a / length;
  return result;
}

DN_API DN_V2F32 DN_V2F32_Perpendicular(DN_V2F32 a)
{
  // NOTE: Matrix form of a 2D vector can be defined as
  //
  // x' = x cos(t) - y sin(t)
  // y' = x sin(t) + y cos(t)
  //
  // Calculate a line perpendicular to a vector means rotating the vector by
  // 90 degrees
  //
  // x' = x cos(90) - y sin(90)
  // y' = x sin(90) + y cos(90)
  //
  // Where `cos(90) = 0` and `sin(90) = 1` then,
  //
  // x' = -y
  // y' = +x

  DN_V2F32 result = DN_V2F32_From2N(-a.y, a.x);
  return result;
}

DN_API DN_V2F32 DN_V2F32_Reflect(DN_V2F32 in, DN_V2F32 surface)
{
  DN_V2F32 normal      = DN_V2F32_Perpendicular(surface);
  DN_V2F32 normal_norm = DN_V2F32_Normalise(normal);
  DN_F32   signed_dist = DN_V2F32_Dot(in, normal_norm);
  DN_V2F32 result      = DN_V2F32_From2N(in.x, in.y + (-signed_dist * 2.f));
  return result;
}

DN_API DN_F32 DN_V2F32_Area(DN_V2F32 a)
{
  DN_F32 result = a.w * a.h;
  return result;
}

DN_API bool operator!=(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator==(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
  return result;
}

DN_API bool operator>=(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y) && (lhs.z >= rhs.z);
  return result;
}

DN_API bool operator<=(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y) && (lhs.z <= rhs.z);
  return result;
}

DN_API bool operator<(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y) && (lhs.z < rhs.z);
  return result;
}

DN_API bool operator>(DN_V3F32 lhs, DN_V3F32 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y) && (lhs.z > rhs.z);
  return result;
}

DN_API DN_V3F32 operator-(DN_V3F32 lhs, DN_V3F32 rhs)
{
  DN_V3F32 result = DN_V3F32_From3N(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
  return result;
}

DN_API DN_V3F32 operator-(DN_V3F32 lhs)
{
  DN_V3F32 result = DN_V3F32_From3N(-lhs.x, -lhs.y, -lhs.z);
  return result;
}

DN_API DN_V3F32 operator+(DN_V3F32 lhs, DN_V3F32 rhs)
{
  DN_V3F32 result = DN_V3F32_From3N(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
  return result;
}

DN_API DN_V3F32 operator*(DN_V3F32 lhs, DN_V3F32 rhs)
{
  DN_V3F32 result = DN_V3F32_From3N(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
  return result;
}

DN_API DN_V3F32 operator*(DN_V3F32 lhs, DN_F32 rhs)
{
  DN_V3F32 result = DN_V3F32_From3N(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
  return result;
}

DN_API DN_V3F32 operator*(DN_V3F32 lhs, int32_t rhs)
{
  DN_V3F32 result = DN_V3F32_From3N(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
  return result;
}

DN_API DN_V3F32 operator/(DN_V3F32 lhs, DN_V3F32 rhs)
{
  DN_V3F32 result = DN_V3F32_From3N(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
  return result;
}

DN_API DN_V3F32 operator/(DN_V3F32 lhs, DN_F32 rhs)
{
  DN_V3F32 result = DN_V3F32_From3N(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
  return result;
}

DN_API DN_V3F32 operator/(DN_V3F32 lhs, int32_t rhs)
{
  DN_V3F32 result = DN_V3F32_From3N(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
  return result;
}

DN_API DN_V3F32 &operator*=(DN_V3F32 &lhs, DN_V3F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V3F32 &operator*=(DN_V3F32 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V3F32 &operator*=(DN_V3F32 &lhs, int32_t rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V3F32 &operator/=(DN_V3F32 &lhs, DN_V3F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V3F32 &operator/=(DN_V3F32 &lhs, DN_F32 rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V3F32 &operator/=(DN_V3F32 &lhs, int32_t rhs)
{
  lhs = lhs / rhs;
  return lhs;
}

DN_API DN_V3F32 &operator-=(DN_V3F32 &lhs, DN_V3F32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V3F32 &operator+=(DN_V3F32 &lhs, DN_V3F32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_F32 DN_V3_LengthSq(DN_V3F32 a)
{
  DN_F32 result = DN_Squared(a.x) + DN_Squared(a.y) + DN_Squared(a.z);
  return result;
}

DN_API DN_F32 DN_V3_Length(DN_V3F32 a)
{
  DN_F32 length_sq = DN_Squared(a.x) + DN_Squared(a.y) + DN_Squared(a.z);
  DN_F32 result    = DN_SqrtF32(length_sq);
  return result;
}

DN_API DN_V3F32 DN_V3_Normalise(DN_V3F32 a)
{
  DN_F32   length = DN_V3_Length(a);
  DN_V3F32 result = a / length;
  return result;
}

DN_API DN_V4F32 DN_V4F32_FromRGBU32(DN_U32 u32)
{
  DN_U8    r      = (DN_U8)((u32 & DN_V4_R_MASK_U32) >> 24);
  DN_U8    g      = (DN_U8)((u32 & DN_V4_G_MASK_U32) >> 16);
  DN_U8    b      = (DN_U8)((u32 & DN_V4_B_MASK_U32) >> 8);
  DN_V4F32 result = DN_V4F32_FromRGBU8(r, g, b);
  return result;
}

DN_API DN_V4F32 DN_V4F32_FromRGBAU32(DN_U32 u32)
{
  DN_U8    r      = (DN_U8)((u32 & DN_V4_R_MASK_U32) >> 24);
  DN_U8    g      = (DN_U8)((u32 & DN_V4_G_MASK_U32) >> 16);
  DN_U8    b      = (DN_U8)((u32 & DN_V4_B_MASK_U32) >> 8);
  DN_U8    a      = (DN_U8)((u32 & DN_V4_A_MASK_U32) >> 0);
  DN_V4F32 result = DN_V4F32_FromRGBAU8(r, g, b, a);
  return result;
}

DN_API bool operator==(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z) && (lhs.w == rhs.w);
  return result;
}

DN_API bool operator!=(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API bool operator>=(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x >= rhs.x) && (lhs.y >= rhs.y) && (lhs.z >= rhs.z) && (lhs.w >= rhs.w);
  return result;
}

DN_API bool operator<=(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x <= rhs.x) && (lhs.y <= rhs.y) && (lhs.z <= rhs.z) && (lhs.w <= rhs.w);
  return result;
}

DN_API bool operator<(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x < rhs.x) && (lhs.y < rhs.y) && (lhs.z < rhs.z) && (lhs.w < rhs.w);
  return result;
}

DN_API bool operator>(DN_V4F32 lhs, DN_V4F32 rhs)
{
  bool result = (lhs.x > rhs.x) && (lhs.y > rhs.y) && (lhs.z > rhs.z) && (lhs.w > rhs.w);
  return result;
}

DN_API DN_V4F32 operator-(DN_V4F32 lhs, DN_V4F32 rhs)
{
  DN_V4F32 result = DN_V4F32_From4N(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
  return result;
}

DN_API DN_V4F32 operator-(DN_V4F32 lhs)
{
  DN_V4F32 result = DN_V4F32_From4N(-lhs.x, -lhs.y, -lhs.z, -lhs.w);
  return result;
}

DN_API DN_V4F32 operator+(DN_V4F32 lhs, DN_V4F32 rhs)
{
  DN_V4F32 result = DN_V4F32_From4N(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
  return result;
}

DN_API DN_V4F32 operator*(DN_V4F32 lhs, DN_V4F32 rhs)
{
  DN_V4F32 result = DN_V4F32_From4N(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
  return result;
}

DN_API DN_V4F32 operator*(DN_V4F32 lhs, DN_F32 rhs)
{
  DN_V4F32 result = DN_V4F32_From4N(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
  return result;
}

DN_API DN_V4F32 operator*(DN_V4F32 lhs, int32_t rhs)
{
  DN_V4F32 result = DN_V4F32_From4N(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
  return result;
}

DN_API DN_V4F32 operator/(DN_V4F32 lhs, DN_F32 rhs)
{
  DN_V4F32 result = DN_V4F32_From4N(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs);
  return result;
}

DN_API DN_V4F32 &operator*=(DN_V4F32 &lhs, DN_V4F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V4F32 &operator*=(DN_V4F32 &lhs, DN_F32 rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V4F32 &operator*=(DN_V4F32 &lhs, int32_t rhs)
{
  lhs = lhs * rhs;
  return lhs;
}

DN_API DN_V4F32 &operator-=(DN_V4F32 &lhs, DN_V4F32 rhs)
{
  lhs = lhs - rhs;
  return lhs;
}

DN_API DN_V4F32 &operator+=(DN_V4F32 &lhs, DN_V4F32 rhs)
{
  lhs = lhs + rhs;
  return lhs;
}

DN_API DN_F32 DN_V4F32Dot(DN_V4F32 a, DN_V4F32 b)
{
  DN_F32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
  return result;
}

DN_API DN_M4 DN_M4_Identity()
{
  DN_M4 result =
      {
          {
           {1, 0, 0, 0},
           {0, 1, 0, 0},
           {0, 0, 1, 0},
           {0, 0, 0, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4_ScaleF(DN_F32 x, DN_F32 y, DN_F32 z)
{
  DN_M4 result =
      {
          {
           {x, 0, 0, 0},
           {0, y, 0, 0},
           {0, 0, z, 0},
           {0, 0, 0, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4_Scale(DN_V3F32 xyz)
{
  DN_M4 result =
      {
          {
           {xyz.x, 0, 0, 0},
           {0, xyz.y, 0, 0},
           {0, 0, xyz.z, 0},
           {0, 0, 0, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4_TranslateF(DN_F32 x, DN_F32 y, DN_F32 z)
{
  DN_M4 result =
      {
          {
           {1, 0, 0, 0},
           {0, 1, 0, 0},
           {0, 0, 1, 0},
           {x, y, z, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4_Translate(DN_V3F32 xyz)
{
  DN_M4 result =
      {
          {
           {1, 0, 0, 0},
           {0, 1, 0, 0},
           {0, 0, 1, 0},
           {xyz.x, xyz.y, xyz.z, 1},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4_Transpose(DN_M4 mat)
{
  DN_M4 result = {};
  for (int col = 0; col < 4; col++)
    for (int row = 0; row < 4; row++)
      result.columns[col][row] = mat.columns[row][col];
  return result;
}

DN_API DN_M4 DN_M4_Rotate(DN_V3F32 axis01, DN_F32 radians)
{
  DN_AssertF(DN_Abs(DN_V3_Length(axis01) - 1.f) <= 0.01f,
             "Rotation axis must be normalised, length = %f",
             DN_V3_Length(axis01));

  DN_F32 sin           = DN_SinF32(radians);
  DN_F32 cos           = DN_CosF32(radians);
  DN_F32 one_minus_cos = 1.f - cos;

  DN_F32 x  = axis01.x;
  DN_F32 y  = axis01.y;
  DN_F32 z  = axis01.z;
  DN_F32 x2 = DN_Squared(x);
  DN_F32 y2 = DN_Squared(y);
  DN_F32 z2 = DN_Squared(z);

  DN_M4 result =
      {
          {
           {cos + x2 * one_minus_cos, y * x * one_minus_cos + z * sin, z * x * one_minus_cos - y * sin, 0}, // Col 1
              {x * y * one_minus_cos - z * sin, cos + y2 * one_minus_cos, z * y * one_minus_cos + x * sin, 0}, // Col 2
              {x * z * one_minus_cos + y * sin, y * z * one_minus_cos - x * sin, cos + z2 * one_minus_cos, 0}, // Col 3
              {0, 0, 0, 1},                                                                                    // Col 4
          }
  };

  return result;
}

DN_API DN_M4 DN_M4_Orthographic(DN_F32 left, DN_F32 right, DN_F32 bottom, DN_F32 top, DN_F32 z_near, DN_F32 z_far)
{
  // NOTE: Here is the matrix in column major for readability. Below it's
  // transposed due to how you have to declare column major matrices in C/C++.
  //
  // m = [2/r-l, 0,      0,     -1*(r+l)/(r-l)]
  //     [0,     2/t-b,  0,      1*(t+b)/(t-b)]
  //     [0,     0,     -2/f-n, -1*(f+n)/(f-n)]
  //     [0,     0,      0,      1            ]

  DN_M4 result =
      {
          {
           {2.f / (right - left), 0.f, 0.f, 0.f},
           {0.f, 2.f / (top - bottom), 0.f, 0.f},
           {0.f, 0.f, -2.f / (z_far - z_near), 0.f},
           {(-1.f * (right + left)) / (right - left), (-1.f * (top + bottom)) / (top - bottom), (-1.f * (z_far + z_near)) / (z_far - z_near), 1.f},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4_Perspective(DN_F32 fov /*radians*/, DN_F32 aspect, DN_F32 z_near, DN_F32 z_far)
{
  DN_F32 tan_fov = DN_TanF32(fov / 2.f);
  DN_M4  result =
      {
          {
           {1.f / (aspect * tan_fov), 0.f, 0.f, 0.f},
           {0, 1.f / tan_fov, 0.f, 0.f},
           {0.f, 0.f, (z_near + z_far) / (z_near - z_far), -1.f},
           {0.f, 0.f, (2.f * z_near * z_far) / (z_near - z_far), 0.f},
           }
  };

  return result;
}

DN_API DN_M4 DN_M4_Add(DN_M4 lhs, DN_M4 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] + rhs.columns[col][it];
  return result;
}

DN_API DN_M4 DN_M4_Sub(DN_M4 lhs, DN_M4 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] - rhs.columns[col][it];
  return result;
}

DN_API DN_M4 DN_M4_Mul(DN_M4 lhs, DN_M4 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      DN_F32 sum = 0;
      for (int f32_it = 0; f32_it < 4; f32_it++)
        sum += lhs.columns[f32_it][row] * rhs.columns[col][f32_it];

      result.columns[col][row] = sum;
    }
  }
  return result;
}

DN_API DN_M4 DN_M4_Div(DN_M4 lhs, DN_M4 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] / rhs.columns[col][it];
  return result;
}

DN_API DN_M4 DN_M4_AddF(DN_M4 lhs, DN_F32 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] + rhs;
  return result;
}

DN_API DN_M4 DN_M4_SubF(DN_M4 lhs, DN_F32 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] - rhs;
  return result;
}

DN_API DN_M4 DN_M4_MulF(DN_M4 lhs, DN_F32 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] * rhs;
  return result;
}

DN_API DN_M4 DN_M4_DivF(DN_M4 lhs, DN_F32 rhs)
{
  DN_M4 result;
  for (int col = 0; col < 4; col++)
    for (int it = 0; it < 4; it++)
      result.columns[col][it] = lhs.columns[col][it] / rhs;
  return result;
}

DN_API DN_Str8x256 DN_M4_ColumnMajorString(DN_M4 mat)
{
  DN_Str8x256 result = {};
  for (int row = 0; row < 4; row++) {
    for (int it = 0; it < 4; it++) {
      if (it == 0)
        DN_FmtAppend(result.data, &result.size, sizeof(result.data), "|");
      DN_FmtAppend(result.data, &result.size, sizeof(result.data), "%.5f", mat.columns[it][row]);
      if (it != 3)
        DN_FmtAppend(result.data, &result.size, sizeof(result.data), ", ");
      else
        DN_FmtAppend(result.data, &result.size, sizeof(result.data), "|\n");
    }
  }
  return result;
}

DN_API bool operator==(DN_M2x3 const &lhs, DN_M2x3 const &rhs)
{
  bool result = DN_Memcmp(lhs.e, rhs.e, sizeof(lhs.e[0]) * DN_ArrayCountU(lhs.e)) == 0;
  return result;
}

DN_API bool operator!=(DN_M2x3 const &lhs, DN_M2x3 const &rhs)
{
  bool result = !(lhs == rhs);
  return result;
}

DN_API DN_M2x3 DN_M2x3_Identity()
{
  DN_M2x3 result = {
      {
       1,
       0,
       0,
       0,
       1,
       0,
       }
  };
  return result;
}

DN_API DN_M2x3 DN_M2x3_Translate(DN_V2F32 offset)
{
  DN_M2x3 result = {
      {
       1,
       0,
       offset.x,
       0,
       1,
       offset.y,
       }
  };
  return result;
}

DN_API DN_M2x3 DN_M2x3_Scale(DN_V2F32 scale)
{
  DN_M2x3 result = {
      {
       scale.x,
       0,
       0,
       0,
       scale.y,
       0,
       }
  };
  return result;
}

DN_API DN_M2x3 DN_M2x3_Rotate(DN_F32 radians)
{
  DN_M2x3 result = {
      {
       DN_CosF32(radians),
       DN_SinF32(radians),
       0,
       -DN_SinF32(radians),
       DN_CosF32(radians),
       0,
       }
  };
  return result;
}

DN_API DN_M2x3 DN_M2x3_Mul(DN_M2x3 m1, DN_M2x3 m2)
{
  // NOTE: Ordinarily you can't multiply M2x3 with M2x3 because column count
  // (3) != row count (2). We pretend we have two 3x3 matrices with the last
  // row set to [0 0 1] and perform a 3x3 matrix multiply.
  //
  // | (0)a (1)b (2)c |   | (0)g (1)h (2)i |
  // | (3)d (4)e (5)f | x | (3)j (4)k (5)l |
  // | (6)0 (7)0 (8)1 |   | (6)0 (7)0 (8)1 |

  DN_M2x3 result = {
      {
          m1.e[0] * m2.e[0] + m1.e[1] * m2.e[3],           // a*g + b*j + c*0[omitted],
          m1.e[0] * m2.e[1] + m1.e[1] * m2.e[4],           // a*h + b*k + c*0[omitted],
          m1.e[0] * m2.e[2] + m1.e[1] * m2.e[5] + m1.e[2], // a*i + b*l + c*1,

          m1.e[3] * m2.e[0] + m1.e[4] * m2.e[3],           // d*g + e*j + f*0[omitted],
          m1.e[3] * m2.e[1] + m1.e[4] * m2.e[4],           // d*h + e*k + f*0[omitted],
          m1.e[3] * m2.e[2] + m1.e[4] * m2.e[5] + m1.e[5], // d*i + e*l + f*1,
      }
  };

  return result;
}

DN_API DN_V2F32 DN_M2x3_Mul2F32(DN_M2x3 m1, DN_F32 x, DN_F32 y)
{
  // NOTE: Ordinarily you can't multiply M2x3 with V2 because column count (3)
  // != row count (2). We pretend we have a V3 with `z` set to `1`.
  //
  // | (0)a (1)b (2)c |   | x |
  // | (3)d (4)e (5)f | x | y |
  //                      | 1 |

  DN_V2F32 result = {
      {
       m1.e[0] * x + m1.e[1] * y + m1.e[2], // a*x + b*y + c*1
          m1.e[3] * x + m1.e[4] * y + m1.e[5], // d*x + e*y + f*1
      }
  };
  return result;
}

DN_API DN_V2F32 DN_M2x3_MulV2(DN_M2x3 m1, DN_V2F32 v2)
{
  DN_V2F32 result = DN_M2x3_Mul2F32(m1, v2.x, v2.y);
  return result;
}

DN_API bool operator==(const DN_Rect &lhs, const DN_Rect &rhs)
{
  bool result = (lhs.pos == rhs.pos) && (lhs.size == rhs.size);
  return result;
}

DN_API DN_V2F32 DN_Rect_Center(DN_Rect rect)
{
  DN_V2F32 result = rect.pos + (rect.size * .5f);
  return result;
}

DN_API bool DN_Rect_ContainsPoint(DN_Rect rect, DN_V2F32 p)
{
  DN_V2F32 min    = rect.pos;
  DN_V2F32 max    = rect.pos + rect.size;
  bool     result = (p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y);
  return result;
}

DN_API bool DN_Rect_ContainsRect(DN_Rect a, DN_Rect b)
{
  DN_V2F32 a_min  = a.pos;
  DN_V2F32 a_max  = a.pos + a.size;
  DN_V2F32 b_min  = b.pos;
  DN_V2F32 b_max  = b.pos + b.size;
  bool     result = (b_min >= a_min && b_max <= a_max);
  return result;
}

DN_API DN_Rect DN_Rect_Expand(DN_Rect a, DN_F32 amount)
{
  DN_Rect result = a;
  result.pos -= amount;
  result.size += (amount * 2.f);
  return result;
}

DN_API DN_Rect DN_Rect_ExpandV2(DN_Rect a, DN_V2F32 amount)
{
  DN_Rect result = a;
  result.pos -= amount;
  result.size += (amount * 2.f);
  return result;
}

DN_API bool DN_Rect_Intersects(DN_Rect a, DN_Rect b)
{
  DN_V2F32 a_min    = a.pos;
  DN_V2F32 a_max    = a.pos + a.size;
  DN_V2F32 b_min    = b.pos;
  DN_V2F32 b_max    = b.pos + b.size;
  bool     has_size = a.size.x && a.size.y && b.size.x && b.size.y;
  bool     result   = false;
  if (has_size)
    result = (a_min.x <= b_max.x && a_max.x >= b_min.x) &&
             (a_min.y <= b_max.y && a_max.y >= b_min.y);
  return result;
}

DN_API DN_Rect DN_Rect_Intersection(DN_Rect a, DN_Rect b)
{
  DN_Rect result = DN_Rect_From2V2(a.pos, DN_V2F32_From1N(0));
  if (DN_Rect_Intersects(a, b)) {
    DN_V2F32 a_min = a.pos;
    DN_V2F32 a_max = a.pos + a.size;
    DN_V2F32 b_min = b.pos;
    DN_V2F32 b_max = b.pos + b.size;

    DN_V2F32 min = {};
    DN_V2F32 max = {};
    min.x        = DN_Max(a_min.x, b_min.x);
    min.y        = DN_Max(a_min.y, b_min.y);
    max.x        = DN_Min(a_max.x, b_max.x);
    max.y        = DN_Min(a_max.y, b_max.y);
    result       = DN_Rect_From2V2(min, max - min);
  }
  return result;
}

DN_API DN_Rect DN_Rect_Union(DN_Rect a, DN_Rect b)
{
  DN_V2F32 a_min = a.pos;
  DN_V2F32 a_max = a.pos + a.size;
  DN_V2F32 b_min = b.pos;
  DN_V2F32 b_max = b.pos + b.size;

  DN_V2F32 min, max;
  min.x          = DN_Min(a_min.x, b_min.x);
  min.y          = DN_Min(a_min.y, b_min.y);
  max.x          = DN_Max(a_max.x, b_max.x);
  max.y          = DN_Max(a_max.y, b_max.y);
  DN_Rect result = DN_Rect_From2V2(min, max - min);
  return result;
}

DN_API DN_RectMinMax DN_Rect_MinMax(DN_Rect a)
{
  DN_RectMinMax result = {};
  result.min           = a.pos;
  result.max           = a.pos + a.size;
  return result;
}

DN_API DN_F32 DN_Rect_Area(DN_Rect a)
{
  DN_F32 result = a.size.w * a.size.h;
  return result;
}

DN_API DN_Rect DN_Rect_CutLeftClip(DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip)
{
  DN_F32 min_x        = rect->pos.x;
  DN_F32 max_x        = rect->pos.x + rect->size.w;
  DN_F32 result_max_x = min_x + amount;
  if (clip)
    result_max_x = DN_Min(result_max_x, max_x);
  DN_Rect result = DN_Rect_From4N(min_x, rect->pos.y, result_max_x - min_x, rect->size.h);
  rect->pos.x    = result_max_x;
  rect->size.w   = max_x - result_max_x;
  return result;
}

DN_API DN_Rect DN_Rect_CutRightClip(DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip)
{
  DN_F32 min_x        = rect->pos.x;
  DN_F32 max_x        = rect->pos.x + rect->size.w;
  DN_F32 result_min_x = max_x - amount;
  if (clip)
    result_min_x = DN_Max(result_min_x, 0);
  DN_Rect result = DN_Rect_From4N(result_min_x, rect->pos.y, max_x - result_min_x, rect->size.h);
  rect->size.w   = result_min_x - min_x;
  return result;
}

DN_API DN_Rect DN_Rect_CutTopClip(DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip)
{
  DN_F32 min_y        = rect->pos.y;
  DN_F32 max_y        = rect->pos.y + rect->size.h;
  DN_F32 result_max_y = min_y + amount;
  if (clip)
    result_max_y = DN_Min(result_max_y, max_y);
  DN_Rect result = DN_Rect_From4N(rect->pos.x, min_y, rect->size.w, result_max_y - min_y);
  rect->pos.y    = result_max_y;
  rect->size.h   = max_y - result_max_y;
  return result;
}

DN_API DN_Rect DN_Rect_CutBottomClip(DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip)
{
  DN_F32 min_y        = rect->pos.y;
  DN_F32 max_y        = rect->pos.y + rect->size.h;
  DN_F32 result_min_y = max_y - amount;
  if (clip)
    result_min_y = DN_Max(result_min_y, 0);
  DN_Rect result = DN_Rect_From4N(rect->pos.x, result_min_y, rect->size.w, max_y - result_min_y);
  rect->size.h   = result_min_y - min_y;
  return result;
}

DN_API DN_Rect DN_RectCut_Cut(DN_RectCut rect_cut, DN_V2F32 size, DN_RectCutClip clip)
{
  DN_Rect result = {};
  if (rect_cut.rect) {
    switch (rect_cut.side) {
      case DN_RectCutSide_Left: result = DN_Rect_CutLeftClip(rect_cut.rect, size.w, clip); break;
      case DN_RectCutSide_Right: result = DN_Rect_CutRightClip(rect_cut.rect, size.w, clip); break;
      case DN_RectCutSide_Top: result = DN_Rect_CutTopClip(rect_cut.rect, size.h, clip); break;
      case DN_RectCutSide_Bottom: result = DN_Rect_CutBottomClip(rect_cut.rect, size.h, clip); break;
    }
  }
  return result;
}

DN_API DN_V2F32 DN_Rect_InterpolatedPoint(DN_Rect rect, DN_V2F32 t01)
{
  DN_V2F32 result = DN_V2F32_From2N(rect.pos.w + (rect.size.w * t01.x),
                                    rect.pos.h + (rect.size.h * t01.y));
  return result;
}

DN_API DN_V2F32 DN_Rect_TopLeft(DN_Rect rect)
{
  DN_V2F32 result = DN_Rect_InterpolatedPoint(rect, DN_V2F32_From2N(0, 0));
  return result;
}

DN_API DN_V2F32 DN_Rect_TopRight(DN_Rect rect)
{
  DN_V2F32 result = DN_Rect_InterpolatedPoint(rect, DN_V2F32_From2N(1, 0));
  return result;
}

DN_API DN_V2F32 DN_Rect_BottomLeft(DN_Rect rect)
{
  DN_V2F32 result = DN_Rect_InterpolatedPoint(rect, DN_V2F32_From2N(0, 1));
  return result;
}

DN_API DN_V2F32 DN_Rect_BottomRight(DN_Rect rect)
{
  DN_V2F32 result = DN_Rect_InterpolatedPoint(rect, DN_V2F32_From2N(1, 1));
  return result;
}

DN_API DN_RaycastLineIntersectV2Result DN_Raycast_LineIntersectV2(DN_V2F32 origin_a, DN_V2F32 dir_a, DN_V2F32 origin_b, DN_V2F32 dir_b)
{
  // NOTE: Parametric equation of a line
  //
  // p = o + (t*d)
  //
  // - o is the starting 2d point
  // - d is the direction of the line
  // - t is a scalar that scales along the direction of the point
  //
  // To determine if a ray intersections a ray, we want to solve
  //
  // (o_a + (t_a * d_a)) = (o_b + (t_b * d_b))
  //
  // Where '_a' and '_b' represent the 1st and 2nd point's origin, direction
  // and 't' components respectively. This is 2 equations with 2 unknowns
  // (`t_a` and `t_b`) which we can solve for by expressing the equation in
  // terms of `t_a` and `t_b`.
  //
  // Working that math out produces the formula below for 't'.

  DN_RaycastLineIntersectV2Result result      = {};
  DN_F32                          denominator = ((dir_b.y * dir_a.x) - (dir_b.x * dir_a.y));
  if (denominator != 0.0f) {
    result.t_a = (((origin_a.y - origin_b.y) * dir_b.x) + ((origin_b.x - origin_a.x) * dir_b.y)) / denominator;
    result.t_b = (((origin_a.y - origin_b.y) * dir_a.x) + ((origin_b.x - origin_a.x) * dir_a.y)) / denominator;
    result.hit = true;
  }
  return result;
}

DN_API DN_V2F32 DN_Lerp_V2F32(DN_V2F32 a, DN_F32 t, DN_V2F32 b)
{
  DN_V2F32 result = {};
  result.x        = a.x + ((b.x - a.x) * t);
  result.y        = a.y + ((b.y - a.y) * t);
  return result;
}

DN_API DN_F32 DN_Lerp_F32(DN_F32 a, DN_F32 t, DN_F32 b)
{
  DN_F32 result = a + ((b - a) * t);
  return result;
}
