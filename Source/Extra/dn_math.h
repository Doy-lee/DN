#if !defined(DN_MATH_H)
#define DN_MATH_H

DN_MSVC_WARNING_PUSH
DN_MSVC_WARNING_DISABLE(4201) // warning C4201: nonstandard extension used: nameless struct/union
#if !defined(DN_NO_V2)
// NOTE: DN_V2 /////////////////////////////////////////////////////////////////////////////////////
union DN_V2I32
{
  struct { int32_t x, y; };
  struct { int32_t w, h; };
  int32_t data[2];
};

union DN_V2U16
{
  struct { uint16_t x, y; };
  struct { uint16_t w, h; };
  uint16_t data[2];
};

union DN_V2F32
{
  struct { DN_F32 x, y; };
  struct { DN_F32 w, h; };
  DN_F32 data[2];
};
#endif // !defined(DN_NO_V2)

#if !defined(DN_NO_V3)
// NOTE: DN_V3 /////////////////////////////////////////////////////////////////////////////////////
union DN_V3F32
{
  struct { DN_F32 x, y, z; };
  struct { DN_F32 r, g, b; };
  DN_F32 data[3];
};

#endif // !defined(DN_NO_V3)

#if !defined(DN_NO_V4)
// NOTE: DN_V4 /////////////////////////////////////////////////////////////////////////////////////
union DN_V4F32
{
  struct { DN_F32 x, y, z, w; };
  struct { DN_F32 r, g, b, a; };
  #if !defined(DN_NO_V3)
  DN_V3F32  rgb;
  DN_V3F32  xyz;
  #endif
  DN_F32 data[4];
};
#endif // !defined(DN_NO_V4)
DN_MSVC_WARNING_POP

#if !defined(DN_NO_M4)
// NOTE: DN_M4 /////////////////////////////////////////////////////////////////////////////////////
struct DN_M4
{
  DN_F32 columns[4][4]; // Column major matrix
};
#endif // !defined(DN_M4)

// NOTE: DN_M2x3 ///////////////////////////////////////////////////////////////////////////////////
union DN_M2x3
{
  DN_F32 e[6];
  DN_F32 row[2][3];
};

// NOTE: DN_Rect ///////////////////////////////////////////////////////////////////////////////////
#if !defined(DN_NO_RECT)
#if defined(DN_NO_V2)
    #error "Rectangles requires V2, DN_NO_V2 must not be defined"
#endif
struct DN_Rect
{
  DN_V2F32 pos, size;
};

struct DN_RectMinMax
{
  DN_V2F32 min, max;
};

enum DN_RectCutClip
{
  DN_RectCutClip_No,
  DN_RectCutClip_Yes,
};

enum DN_RectCutSide
{
  DN_RectCutSide_Left,
  DN_RectCutSide_Right,
  DN_RectCutSide_Top,
  DN_RectCutSide_Bottom,
};

struct DN_RectCut
{
  DN_Rect*       rect;
  DN_RectCutSide side;
};
#endif // !defined(DN_NO_RECT)

// NOTE: Other /////////////////////////////////////////////////////////////////////////////////////
// NOTE: API
struct DN_RaycastLineIntersectV2Result
{
    bool    hit; // True if there was an intersection, false if the lines are parallel
    DN_F32 t_a; // Distance along `dir_a` that the intersection occurred, e.g. `origin_a + (dir_a * t_a)`
    DN_F32 t_b; // Distance along `dir_b` that the intersection occurred, e.g. `origin_b + (dir_b * t_b)`
};

#if !defined(DN_NO_V2)
// NOTE: DN_V2 /////////////////////////////////////////////////////////////////////////////////////
#define              DN_V2I32_Zero                               DN_LITERAL(DN_V2I32){{(int32_t)(0),    (int32_t)(0)}}
#define              DN_V2I32_One                                DN_LITERAL(DN_V2I32){{(int32_t)(1),    (int32_t)(1)}}
#define              DN_V2I32_From1N(x)                          DN_LITERAL(DN_V2I32){{(int32_t)(x),    (int32_t)(x)}}
#define              DN_V2I32_From2N(x, y)                       DN_LITERAL(DN_V2I32){{(int32_t)(x),    (int32_t)(y)}}
#define              DN_V2I32_InitV2(xy)                         DN_LITERAL(DN_V2I32){{(int32_t)(xy).x, (int32_t)(xy).y}}

DN_API bool          operator!=                                  (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool          operator==                                  (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool          operator>=                                  (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool          operator<=                                  (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool          operator<                                   (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API bool          operator>                                   (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32      operator-                                   (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32      operator-                                   (DN_V2I32                 lhs);
DN_API DN_V2I32      operator+                                   (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32      operator*                                   (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32      operator*                                   (DN_V2I32  lhs, DN_F32   rhs);
DN_API DN_V2I32      operator*                                   (DN_V2I32  lhs, int32_t   rhs);
DN_API DN_V2I32      operator/                                   (DN_V2I32  lhs, DN_V2I32 rhs);
DN_API DN_V2I32      operator/                                   (DN_V2I32  lhs, DN_F32   rhs);
DN_API DN_V2I32      operator/                                   (DN_V2I32  lhs, int32_t   rhs);
DN_API DN_V2I32 &    operator*=                                  (DN_V2I32& lhs, DN_V2I32 rhs);
DN_API DN_V2I32 &    operator*=                                  (DN_V2I32& lhs, DN_F32   rhs);
DN_API DN_V2I32 &    operator*=                                  (DN_V2I32& lhs, int32_t   rhs);
DN_API DN_V2I32 &    operator/=                                  (DN_V2I32& lhs, DN_V2I32 rhs);
DN_API DN_V2I32 &    operator/=                                  (DN_V2I32& lhs, DN_F32   rhs);
DN_API DN_V2I32 &    operator/=                                  (DN_V2I32& lhs, int32_t   rhs);
DN_API DN_V2I32 &    operator-=                                  (DN_V2I32& lhs, DN_V2I32 rhs);
DN_API DN_V2I32 &    operator+=                                  (DN_V2I32& lhs, DN_V2I32 rhs);

DN_API DN_V2I32      DN_V2I32_Min                                (DN_V2I32 a, DN_V2I32 b);
DN_API DN_V2I32      DN_V2I32_Max                                (DN_V2I32 a, DN_V2I32 b);
DN_API DN_V2I32      DN_V2I32_Abs                                (DN_V2I32 a);

#define              DN_V2U16_Zero                               DN_LITERAL(DN_V2U16){{(uint16_t)(0), (uint16_t)(0)}}
#define              DN_V2U16_One                                DN_LITERAL(DN_V2U16){{(uint16_t)(1), (uint16_t)(1)}}
#define              DN_V2U16_From1N(x)                          DN_LITERAL(DN_V2U16){{(uint16_t)(x), (uint16_t)(x)}}
#define              DN_V2U16_From2N(x, y)                       DN_LITERAL(DN_V2U16){{(uint16_t)(x), (uint16_t)(y)}}

DN_API bool          operator!=                                  (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool          operator==                                  (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool          operator>=                                  (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool          operator<=                                  (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool          operator<                                   (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API bool          operator>                                   (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16      operator-                                   (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16      operator+                                   (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16      operator*                                   (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16      operator*                                   (DN_V2U16  lhs, DN_F32 rhs);
DN_API DN_V2U16      operator*                                   (DN_V2U16  lhs, int32_t rhs);
DN_API DN_V2U16      operator/                                   (DN_V2U16  lhs, DN_V2U16 rhs);
DN_API DN_V2U16      operator/                                   (DN_V2U16  lhs, DN_F32 rhs);
DN_API DN_V2U16      operator/                                   (DN_V2U16  lhs, int32_t rhs);
DN_API DN_V2U16 &    operator*=                                  (DN_V2U16& lhs, DN_V2U16 rhs);
DN_API DN_V2U16 &    operator*=                                  (DN_V2U16& lhs, DN_F32 rhs);
DN_API DN_V2U16 &    operator*=                                  (DN_V2U16& lhs, int32_t rhs);
DN_API DN_V2U16 &    operator/=                                  (DN_V2U16& lhs, DN_V2U16 rhs);
DN_API DN_V2U16 &    operator/=                                  (DN_V2U16& lhs, DN_F32 rhs);
DN_API DN_V2U16 &    operator/=                                  (DN_V2U16& lhs, int32_t rhs);
DN_API DN_V2U16 &    operator-=                                  (DN_V2U16& lhs, DN_V2U16 rhs);
DN_API DN_V2U16 &    operator+=                                  (DN_V2U16& lhs, DN_V2U16 rhs);

#define              DN_V2F32_Zero                               DN_LITERAL(DN_V2F32){{(DN_F32)(0),    (DN_F32)(0)}}
#define              DN_V2F32_One                                DN_LITERAL(DN_V2F32){{(DN_F32)(1),    (DN_F32)(1)}}
#define              DN_V2F32_From1N(x)                          DN_LITERAL(DN_V2F32){{(DN_F32)(x),    (DN_F32)(x)}}
#define              DN_V2F32_From2N(x, y)                       DN_LITERAL(DN_V2F32){{(DN_F32)(x),    (DN_F32)(y)}}
#define              DN_V2F32_FromV2I32(xy)                      DN_LITERAL(DN_V2F32){{(DN_F32)(xy).x, (DN_F32)(xy).y}}

DN_API bool          operator!=                                  (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool          operator==                                  (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool          operator>=                                  (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool          operator<=                                  (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool          operator<                                   (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API bool          operator>                                   (DN_V2F32  lhs, DN_V2F32  rhs);

DN_API DN_V2F32      operator-                                   (DN_V2F32  lhs);
DN_API DN_V2F32      operator-                                   (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API DN_V2F32      operator-                                   (DN_V2F32  lhs, DN_V2I32  rhs);
DN_API DN_V2F32      operator-                                   (DN_V2F32  lhs, DN_F32    rhs);
DN_API DN_V2F32      operator-                                   (DN_V2F32  lhs, int32_t    rhs);

DN_API DN_V2F32      operator+                                   (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API DN_V2F32      operator+                                   (DN_V2F32  lhs, DN_V2I32  rhs);
DN_API DN_V2F32      operator+                                   (DN_V2F32  lhs, DN_F32    rhs);
DN_API DN_V2F32      operator+                                   (DN_V2F32  lhs, int32_t    rhs);

DN_API DN_V2F32      operator*                                   (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API DN_V2F32      operator*                                   (DN_V2F32  lhs, DN_V2I32  rhs);
DN_API DN_V2F32      operator*                                   (DN_V2F32  lhs, DN_F32    rhs);
DN_API DN_V2F32      operator*                                   (DN_V2F32  lhs, int32_t    rhs);

DN_API DN_V2F32      operator/                                   (DN_V2F32  lhs, DN_V2F32  rhs);
DN_API DN_V2F32      operator/                                   (DN_V2F32  lhs, DN_V2I32  rhs);
DN_API DN_V2F32      operator/                                   (DN_V2F32  lhs, DN_F32    rhs);
DN_API DN_V2F32      operator/                                   (DN_V2F32  lhs, int32_t    rhs);

DN_API DN_V2F32 &    operator*=                                  (DN_V2F32&  lhs, DN_V2F32 rhs);
DN_API DN_V2F32 &    operator*=                                  (DN_V2F32&  lhs, DN_V2I32 rhs);
DN_API DN_V2F32 &    operator*=                                  (DN_V2F32&  lhs, DN_F32   rhs);
DN_API DN_V2F32 &    operator*=                                  (DN_V2F32&  lhs, int32_t   rhs);

DN_API DN_V2F32 &    operator/=                                  (DN_V2F32&  lhs, DN_V2F32 rhs);
DN_API DN_V2F32 &    operator/=                                  (DN_V2F32&  lhs, DN_V2I32 rhs);
DN_API DN_V2F32 &    operator/=                                  (DN_V2F32&  lhs, DN_F32   rhs);
DN_API DN_V2F32 &    operator/=                                  (DN_V2F32&  lhs, int32_t   rhs);

DN_API DN_V2F32 &    operator-=                                  (DN_V2F32&  lhs, DN_V2F32 rhs);
DN_API DN_V2F32 &    operator-=                                  (DN_V2F32&  lhs, DN_V2I32 rhs);
DN_API DN_V2F32 &    operator-=                                  (DN_V2F32&  lhs, DN_F32   rhs);
DN_API DN_V2F32 &    operator-=                                  (DN_V2F32&  lhs, int32_t   rhs);

DN_API DN_V2F32 &    operator+=                                  (DN_V2F32&  lhs, DN_V2F32 rhs);
DN_API DN_V2F32 &    operator+=                                  (DN_V2F32&  lhs, DN_V2I32 rhs);
DN_API DN_V2F32 &    operator+=                                  (DN_V2F32&  lhs, DN_F32   rhs);
DN_API DN_V2F32 &    operator+=                                  (DN_V2F32&  lhs, int32_t   rhs);

DN_API DN_V2F32      DN_V2F32_Min                                (DN_V2F32 a, DN_V2F32 b);
DN_API DN_V2F32      DN_V2F32_Max                                (DN_V2F32 a, DN_V2F32 b);
DN_API DN_V2F32      DN_V2F32_Abs                                (DN_V2F32 a);
DN_API DN_F32        DN_V2F32_Dot                                (DN_V2F32 a, DN_V2F32 b);
DN_API DN_F32        DN_V2F32_LengthSq2V2                        (DN_V2F32 lhs, DN_V2F32 rhs);
DN_API bool          DN_V2F32_LengthSqIsWithin2V2                (DN_V2F32 lhs, DN_V2F32 rhs, DN_F32 within_amount_sq);
DN_API DN_F32        DN_V2F32_Length2V2                          (DN_V2F32 lhs, DN_V2F32 rhs);
DN_API DN_F32        DN_V2F32_LengthSq                           (DN_V2F32 lhs);
DN_API DN_F32        DN_V2F32_Length                             (DN_V2F32 lhs);
DN_API DN_V2F32      DN_V2F32_Normalise                          (DN_V2F32 a);
DN_API DN_V2F32      DN_V2F32_Perpendicular                      (DN_V2F32 a);
DN_API DN_V2F32      DN_V2F32_Reflect                            (DN_V2F32 in, DN_V2F32 surface);
DN_API DN_F32        DN_V2F32_Area                               (DN_V2F32 a);
#endif // !defined(DN_NO_V2)
#if !defined(DN_NO_V3)
// NOTE: DN_V3 /////////////////////////////////////////////////////////////////////////////////////
#define              DN_V3F32_From1N(x)                          DN_LITERAL(DN_V3F32){{(DN_F32)(x),    (DN_F32)(x),    (DN_F32)(x)}}
#define              DN_V3F32_From3N(x, y, z)                    DN_LITERAL(DN_V3F32){{(DN_F32)(x),    (DN_F32)(y),    (DN_F32)(z)}}
#define              DN_V3F32_FromV2F32And1N(xy, z)              DN_LITERAL(DN_V3F32){{(DN_F32)(xy.x), (DN_F32)(xy.y), (DN_F32)(z)}}

DN_API bool          operator==                                  (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool          operator!=                                  (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool          operator>=                                  (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool          operator<=                                  (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool          operator<                                   (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API bool          operator>                                   (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API DN_V3F32      operator-                                   (DN_V3F32  lhs, DN_V3F32  rhs);
DN_API DN_V3F32      operator-                                   (DN_V3F32  lhs);
DN_API DN_V3F32      operator+                                   (DN_V3F32  lhs, DN_V3F32 rhs);
DN_API DN_V3F32      operator*                                   (DN_V3F32  lhs, DN_V3F32 rhs);
DN_API DN_V3F32      operator*                                   (DN_V3F32  lhs, DN_F32   rhs);
DN_API DN_V3F32      operator*                                   (DN_V3F32  lhs, int32_t   rhs);
DN_API DN_V3F32      operator/                                   (DN_V3F32  lhs, DN_V3F32 rhs);
DN_API DN_V3F32      operator/                                   (DN_V3F32  lhs, DN_F32   rhs);
DN_API DN_V3F32      operator/                                   (DN_V3F32  lhs, int32_t   rhs);
DN_API DN_V3F32 &    operator*=                                  (DN_V3F32 &lhs, DN_V3F32 rhs);
DN_API DN_V3F32 &    operator*=                                  (DN_V3F32 &lhs, DN_F32   rhs);
DN_API DN_V3F32 &    operator*=                                  (DN_V3F32 &lhs, int32_t   rhs);
DN_API DN_V3F32 &    operator/=                                  (DN_V3F32 &lhs, DN_V3F32 rhs);
DN_API DN_V3F32 &    operator/=                                  (DN_V3F32 &lhs, DN_F32   rhs);
DN_API DN_V3F32 &    operator/=                                  (DN_V3F32 &lhs, int32_t   rhs);
DN_API DN_V3F32 &    operator-=                                  (DN_V3F32 &lhs, DN_V3F32 rhs);
DN_API DN_V3F32 &    operator+=                                  (DN_V3F32 &lhs, DN_V3F32 rhs);
DN_API DN_F32        DN_V3F32_LengthSq                           (DN_V3F32 a);
DN_API DN_F32        DN_V3F32_Length                             (DN_V3F32 a);
DN_API DN_V3F32      DN_V3F32_Normalise                          (DN_V3F32 a);
#endif // !defined(DN_NO_V3)
#if !defined(DN_NO_V4)
DN_U32 const DN_V4_R_MASK_U32 = 0xFF000000;
DN_U32 const DN_V4_G_MASK_U32 = 0x00FF0000;
DN_U32 const DN_V4_B_MASK_U32 = 0x0000FF00;
DN_U32 const DN_V4_A_MASK_U32 = 0x000000FF;

// NOTE: DN_V4 /////////////////////////////////////////////////////////////////////////////////////
#define              DN_V4F32_From1N(x)                          DN_LITERAL(DN_V4F32){{(DN_F32)(x), (DN_F32)(x), (DN_F32)(x), (DN_F32)(x)}}
#define              DN_V4F32_From4N(x, y, z, w)                 DN_LITERAL(DN_V4F32){{(DN_F32)(x), (DN_F32)(y), (DN_F32)(z), (DN_F32)(w)}}
#define              DN_V4F32_FromV3And1N(xyz, w)                DN_LITERAL(DN_V4F32){{xyz.x,        xyz.y,        xyz.z,        w}}
#define              DN_V4F32_FromRGBAU8(r, g, b, a)             DN_LITERAL(DN_V4F32){{r / 255.f, g / 255.f, b / 255.f, a / 255.f}}
#define              DN_V4F32_FromRGBU8(r, g, b)                 DN_LITERAL(DN_V4F32){{r / 255.f, g / 255.f, b / 255.f,       1.f}}
DN_API DN_V4F32      DN_V4F32_FromRGBU32(DN_U32 u32);
DN_API DN_V4F32      DN_V4F32_FromRGBAU32(DN_U32 u32);
#define              DN_V4F32_FromV4Alpha(v4, alpha)             DN_V4F32_FromV3And1N(v4.xyz, alpha)
DN_API bool          operator==                                  (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API bool          operator!=                                  (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API bool          operator<=                                  (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API bool          operator<                                   (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API bool          operator>                                   (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API DN_V4F32      operator-                                   (DN_V4F32  lhs, DN_V4F32  rhs);
DN_API DN_V4F32      operator-                                   (DN_V4F32  lhs);
DN_API DN_V4F32      operator+                                   (DN_V4F32  lhs, DN_V4F32 rhs);
DN_API DN_V4F32      operator*                                   (DN_V4F32  lhs, DN_V4F32 rhs);
DN_API DN_V4F32      operator*                                   (DN_V4F32  lhs, DN_F32   rhs);
DN_API DN_V4F32      operator*                                   (DN_V4F32  lhs, int32_t   rhs);
DN_API DN_V4F32      operator/                                   (DN_V4F32  lhs, DN_F32   rhs);
DN_API DN_V4F32 &    operator*=                                  (DN_V4F32 &lhs, DN_V4F32 rhs);
DN_API DN_V4F32 &    operator*=                                  (DN_V4F32 &lhs, DN_F32   rhs);
DN_API DN_V4F32 &    operator*=                                  (DN_V4F32 &lhs, int32_t   rhs);
DN_API DN_V4F32 &    operator-=                                  (DN_V4F32 &lhs, DN_V4F32 rhs);
DN_API DN_V4F32 &    operator+=                                  (DN_V4F32 &lhs, DN_V4F32 rhs);
DN_API DN_F32        DN_V4F32_Dot                                (DN_V4F32 a, DN_V4F32 b);
#endif // !defined(DN_NO_V4)
#if !defined(DN_NO_M4)
// NOTE: DN_M4 /////////////////////////////////////////////////////////////////////////////////////
DN_API DN_M4         DN_M4_Identity                              ();
DN_API DN_M4         DN_M4_ScaleF                                (DN_F32 x, DN_F32 y, DN_F32 z);
DN_API DN_M4         DN_M4_Scale                                 (DN_V3F32 xyz);
DN_API DN_M4         DN_M4_TranslateF                            (DN_F32 x, DN_F32 y, DN_F32 z);
DN_API DN_M4         DN_M4_Translate                             (DN_V3F32 xyz);
DN_API DN_M4         DN_M4_Transpose                             (DN_M4 mat);
DN_API DN_M4         DN_M4_Rotate                                (DN_V3F32 axis, DN_F32 radians);
DN_API DN_M4         DN_M4_Orthographic                          (DN_F32 left, DN_F32 right, DN_F32 bottom, DN_F32 top, DN_F32 z_near, DN_F32 z_far);
DN_API DN_M4         DN_M4_Perspective                           (DN_F32 fov /*radians*/, DN_F32 aspect, DN_F32 z_near, DN_F32 z_far);
DN_API DN_M4         DN_M4_Add                                   (DN_M4 lhs, DN_M4 rhs);
DN_API DN_M4         DN_M4_Sub                                   (DN_M4 lhs, DN_M4 rhs);
DN_API DN_M4         DN_M4_Mul                                   (DN_M4 lhs, DN_M4 rhs);
DN_API DN_M4         DN_M4_Div                                   (DN_M4 lhs, DN_M4 rhs);
DN_API DN_M4         DN_M4_AddF                                  (DN_M4 lhs, DN_F32 rhs);
DN_API DN_M4         DN_M4_SubF                                  (DN_M4 lhs, DN_F32 rhs);
DN_API DN_M4         DN_M4_MulF                                  (DN_M4 lhs, DN_F32 rhs);
DN_API DN_M4         DN_M4_DivF                                  (DN_M4 lhs, DN_F32 rhs);
DN_API DN_Str8x256   DN_M4_ColumnMajorString                     (DN_M4 mat);
#endif // !defined(DN_NO_M4)
// NOTE: DN_M2x3 ///////////////////////////////////////////////////////////////////////////////////
DN_API bool          operator==                                  (DN_M2x3 const &lhs, DN_M2x3 const &rhs);
DN_API bool          operator!=                                  (DN_M2x3 const &lhs, DN_M2x3 const &rhs);
DN_API DN_M2x3       DN_M2x3_Identity                            ();
DN_API DN_M2x3       DN_M2x3_Translate                           (DN_V2F32 offset);
DN_API DN_M2x3       DN_M2x3_Scale                               (DN_V2F32 scale);
DN_API DN_M2x3       DN_M2x3_Rotate                              (DN_F32 radians);
DN_API DN_M2x3       DN_M2x3_Mul                                 (DN_M2x3 m1, DN_M2x3 m2);
DN_API DN_V2F32      DN_M2x3_Mul2F32                             (DN_M2x3 m1, DN_F32 x, DN_F32 y);
DN_API DN_V2F32      DN_M2x3_MulV2                               (DN_M2x3 m1, DN_V2F32 v2);
#if !defined(DN_NO_RECT)
// NOTE: DN_Rect ///////////////////////////////////////////////////////////////////////////////////
#define              DN_Rect_From2V2(pos, size)                  DN_LITERAL(DN_Rect){(pos), (size)}
#define              DN_Rect_From4N(x, y, w, h)                  DN_LITERAL(DN_Rect){DN_LITERAL(DN_V2F32){{x, y}}, DN_LITERAL(DN_V2F32){{w, h}}}

DN_API bool          operator==                                  (const DN_Rect& lhs, const DN_Rect& rhs);
DN_API DN_V2F32      DN_Rect_Center                              (DN_Rect rect);
DN_API bool          DN_Rect_ContainsPoint                       (DN_Rect rect, DN_V2F32 p);
DN_API bool          DN_Rect_ContainsRect                        (DN_Rect a, DN_Rect b);
DN_API DN_Rect       DN_Rect_Expand                              (DN_Rect a, DN_F32 amount);
DN_API DN_Rect       DN_Rect_ExpandV2                            (DN_Rect a, DN_V2F32 amount);
DN_API bool          DN_Rect_Intersects                          (DN_Rect a, DN_Rect b);
DN_API DN_Rect       DN_Rect_Intersection                        (DN_Rect a, DN_Rect b);
DN_API DN_Rect       DN_Rect_Union                               (DN_Rect a, DN_Rect b);
DN_API DN_RectMinMax DN_Rect_MinMax                              (DN_Rect a);
DN_API DN_F32        DN_Rect_Area                                (DN_Rect a);
DN_API DN_V2F32      DN_Rect_InterpolatedPoint                   (DN_Rect rect, DN_V2F32 t01);
DN_API DN_V2F32      DN_Rect_TopLeft                             (DN_Rect rect);
DN_API DN_V2F32      DN_Rect_TopRight                            (DN_Rect rect);
DN_API DN_V2F32      DN_Rect_BottomLeft                          (DN_Rect rect);
DN_API DN_V2F32      DN_Rect_BottomRight                         (DN_Rect rect);

DN_API DN_Rect       DN_Rect_CutLeftClip                         (DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip);
DN_API DN_Rect       DN_Rect_CutRightClip                        (DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip);
DN_API DN_Rect       DN_Rect_CutTopClip                          (DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip);
DN_API DN_Rect       DN_Rect_CutBottomClip                       (DN_Rect *rect, DN_F32 amount, DN_RectCutClip clip);

#define              DN_Rect_CutLeft(rect, amount)               DN_Rect_CutLeftClip(rect, amount, DN_RectCutClip_Yes)
#define              DN_Rect_CutRight(rect, amount)              DN_Rect_CutRightClip(rect, amount, DN_RectCutClip_Yes)
#define              DN_Rect_CutTop(rect, amount)                DN_Rect_CutTopClip(rect, amount, DN_RectCutClip_Yes)
#define              DN_Rect_CutBottom(rect, amount)             DN_Rect_CutBottomClip(rect, amount, DN_RectCutClip_Yes)

#define              DN_Rect_CutLeftNoClip(rect, amount)         DN_Rect_CutLeftClip(rect, amount, DN_RectCutClip_No)
#define              DN_Rect_CutRightNoClip(rect, amount)        DN_Rect_CutRightClip(rect, amount, DN_RectCutClip_No)
#define              DN_Rect_CutTopNoClip(rect, amount)          DN_Rect_CutTopClip(rect, amount, DN_RectCutClip_No)
#define              DN_Rect_CutBottomNoClip(rect, amount)       DN_Rect_CutBottomClip(rect, amount, DN_RectCutClip_No)

DN_API DN_Rect       DN_RectCut_Cut                              (DN_RectCut rect_cut, DN_V2F32 size, DN_RectCutClip clip);
#define              DN_RectCut_Init(rect, side)                 DN_LITERAL(DN_RectCut){rect, side}
#define              DN_RectCut_Left(rect)                       DN_LITERAL(DN_RectCut){rect, DN_RectCutSide_Left}
#define              DN_RectCut_Right(rect)                      DN_LITERAL(DN_RectCut){rect, DN_RectCutSide_Right}
#define              DN_RectCut_Top(rect)                        DN_LITERAL(DN_RectCut){rect, DN_RectCutSide_Top}
#define              DN_RectCut_Bottom(rect)                     DN_LITERAL(DN_RectCut){rect, DN_RectCutSide_Bottom}
#endif // !defined(DN_NO_RECT)
// NOTE: Other /////////////////////////////////////////////////////////////////////////////////////
DN_API DN_RaycastLineIntersectV2Result DN_Raycast_LineIntersectV2(DN_V2F32 origin_a, DN_V2F32 dir_a, DN_V2F32 origin_b, DN_V2F32 dir_b);
DN_API DN_V2F32                        DN_Lerp_V2F32             (DN_V2F32 a, DN_F32 t, DN_V2F32 b);
DN_API DN_F32                          DN_Lerp_F32               (DN_F32 a, DN_F32 t, DN_F32 b);

#endif // !defined(DN_MATH_H)
