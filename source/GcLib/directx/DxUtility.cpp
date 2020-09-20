#include "source/GcLib/pch.h"

#include "DxUtility.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//ColorAccess
**********************************************************/
D3DCOLORVALUE ColorAccess::SetColor(D3DCOLORVALUE& value, D3DCOLOR color) {
	D3DXVECTOR4 _col = ToVec4Normalized(color);							//argb
	D3DXVECTOR4 colVec = D3DXVECTOR4(_col.y, _col.z, _col.w, _col.x);	//rgba
	value = (D3DCOLORVALUE&)ColorAccess::SetColor(colVec, color);
	return value;
}
D3DMATERIAL9 ColorAccess::SetColor(D3DMATERIAL9 mat, D3DCOLOR color) {
	D3DXVECTOR4 _col = ToVec4Normalized(color);					//argb
	__m128 col = Vectorize::Set(_col.y, _col.z, _col.w, _col.x);	//rgba
	__m128 dst;

	auto PerformMul = [&](D3DCOLORVALUE* src) {
		memcpy(&dst, src, sizeof(D3DCOLORVALUE));	//rgba
		dst = Vectorize::Mul(Vectorize::Load((float*)src), col);
		Vectorize::Store((float*)src, dst);
	};
	PerformMul(&mat.Diffuse);
	PerformMul(&mat.Specular);
	PerformMul(&mat.Ambient);
	PerformMul(&mat.Emissive);

	return mat;
}
D3DXVECTOR4& ColorAccess::SetColor(D3DXVECTOR4& value, D3DCOLOR color) {
	__m128 v1 = Vectorize::Load(&value.x);					//argb
	__m128 col = Vectorize::Load(ToVec4Normalized(color));	//argb
	v1 = Vectorize::Mul(v1, col);
	value = ColorAccess::ClampColorPacked((__m128i&)v1);
	return value;
}
D3DCOLOR& ColorAccess::SetColor(D3DCOLOR& src, const D3DCOLOR& mul) {
	D3DXVECTOR4 mulFac = ToVec4Normalized(mul);
	__m128 vsrc = Vectorize::Load(ToVec4(src));			//argb
	__m128 res = Vectorize::Mul(vsrc, Vectorize::Load(mulFac));
	__m128i argb = ColorAccess::ClampColorPackedM((D3DXVECTOR4&)res);
	src = D3DCOLOR_ARGB(argb.m128i_i32[0], argb.m128i_i32[1], argb.m128i_i32[2], argb.m128i_i32[3]);
	return src;
}
D3DCOLOR& ColorAccess::ApplyAlpha(D3DCOLOR& color, float alpha) {
	__m128 v1 = Vectorize::Load(ToVec4(color));			//argb
	__m128 v2 = Vectorize::Replicate(alpha);
	v1 = Vectorize::Mul(v1, v2);
	__m128i ci = ColorAccess::ClampColorPackedM((D3DXVECTOR4&)v1);
	color = D3DCOLOR_ARGB(ci.m128i_i32[0], ci.m128i_i32[1], ci.m128i_i32[2], ci.m128i_i32[3]);
	return color;
}

D3DXVECTOR4 ColorAccess::ClampColorPacked(const D3DXVECTOR4& src) {
	__m128i ci = Vectorize::Set((int)src[0], (int)src[1], (int)src[2], (int)src[3]);
	return ColorAccess::ClampColorPacked(ci);
}
D3DXVECTOR4 ColorAccess::ClampColorPacked(const __m128i& src) {
	__m128i ci = ColorAccess::ClampColorPackedM(src);
	return D3DXVECTOR4(ci.m128i_i32[0], ci.m128i_i32[1], ci.m128i_i32[2], ci.m128i_i32[3]);
}
__m128i ColorAccess::ClampColorPackedM(const D3DXVECTOR4& src) {
	__m128i ci = Vectorize::Set((int)src[0], (int)src[1], (int)src[2], (int)src[3]);
	return ColorAccess::ClampColorPackedM(ci);
}
__m128i ColorAccess::ClampColorPackedM(const __m128i& src) {
	return (__m128i&)Vectorize::ClampPacked(src, 
		Vectorize::Replicate(0x00), Vectorize::Replicate(0xff));
}

D3DXVECTOR3& ColorAccess::RGBtoHSV(D3DXVECTOR3& color, int red, int green, int blue) {
	//[In]  RGB: (0 ~ 255, 0 ~ 255, 0 ~ 255)
	//[Out] HSV: (0 ~ 360, 0 ~ 255, 0 ~ 255)

	int cmax = std::max(std::max(red, green), blue);
	int cmin = std::min(std::min(red, green), blue);
	float delta = cmax - cmin;

	color.z = cmax;
	if (delta < 0.00001f) {
		color.x = 0;
		color.y = 0;
	}
	else if (cmax == 0) {
		color.x = NAN;
		color.y = 0;
	}
	else {
		color.y = (int)std::roundf(delta / cmax * 255.0f);

		if (red >= cmax)
			color.x = (green - blue) / delta;
		else if (green >= cmax)
			color.x = 2.0f + (blue - red) / delta;
		else
			color.x = 4.0f + (red - green) / delta;

		color.x = (int)Math::NormalizeAngleDeg(color.x * 60.0f);
	}

	return color;
}
D3DCOLOR& ColorAccess::HSVtoRGB(D3DCOLOR& color, int hue, int saturation, int value) {
	//Hue: 0 ~ 360
	//Sat: 0 ~ 255
	//Val: 0 ~ 255

	int hh = hue % 360;
	if (hh < 0) hh += 360;

	int i = hh / 60;
	float ff = (hh % 60) / 60.0f;
	float s = saturation / 255.0f;

	int p = value * (1.0f - s);
	int q = value * (1.0f - s * ff);
	int t = value * (1.0f - s * (1.0f - ff));

	auto GenColor = [](int r, int g, int b) -> D3DCOLOR {
		__m128i ci = Vectorize::Set(r, g, b, 0);
		ci = ColorAccess::ClampColorPackedM(ci);
		return D3DCOLOR_XRGB(ci.m128i_i32[0], ci.m128i_i32[1], ci.m128i_i32[2]);
	};

	D3DCOLOR rgb = 0xffffffff;
	switch (i) {
	case 0: rgb = GenColor(value, t, p); break;
	case 1: rgb = GenColor(q, value, p); break;
	case 2: rgb = GenColor(p, value, t); break;
	case 3: rgb = GenColor(p, q, value); break;
	case 4: rgb = GenColor(t, p, value); break;
	case 5: 
	default:
			rgb = GenColor(value, p, q); break;
	}

	color = (rgb & 0x00ffffff) | (color & 0xff000000);
	return color;
}

D3DXVECTOR4 ColorAccess::ToVec4(const D3DCOLOR& color) {
	return D3DXVECTOR4(GetColorA(color), GetColorR(color), GetColorG(color), GetColorB(color));
}
D3DXVECTOR4 ColorAccess::ToVec4Normalized(const D3DCOLOR& color) {
	__m128 argb = Vectorize::Load(ToVec4(color));
	__m128 nor = Vectorize::Replicate(1.0f / 255.0f);
	nor = Vectorize::Mul(argb, nor);
	return (D3DXVECTOR4&)nor;
}
D3DCOLOR ColorAccess::ToD3DCOLOR(const __m128i& col) {
	return D3DCOLOR_ARGB(col.m128i_i32[0], col.m128i_i32[1], col.m128i_i32[2], col.m128i_i32[3]);
}
D3DCOLOR ColorAccess::ToD3DCOLOR(const D3DXVECTOR4& color) {
	return D3DCOLOR_ARGB((byte)color.x, (byte)color.y, (byte)color.z, (byte)color.w);
}

/**********************************************************
//DxMath
**********************************************************/
bool DxMath::IsIntersected(D3DXVECTOR2& pos, std::vector<D3DXVECTOR2>& list) {
	if (list.size() <= 2) return false;

	bool res = true;
	for (size_t iPos = 0; iPos < list.size(); ++iPos) {
		size_t p1 = iPos;
		size_t p2 = (iPos + 1) % list.size();

		float cross_x = (list[p2].x - list[p1].x) * (pos.y - list[p1].y);
		float cross_y = (list[p2].y - list[p1].y) * (pos.x - list[p1].x);
		if (cross_x - cross_y < 0) res = false;
	}
	return res;
}
bool DxMath::IsIntersected(DxCircle& circle1, DxCircle& circle2) {
	float rx = circle1.GetX() - circle2.GetX();
	float ry = circle1.GetY() - circle2.GetY();
	float rr = circle1.GetR() + circle2.GetR();
#ifdef __L_MATH_VECTORIZE
	__m128 v1 = Vectorize::Set(rx, ry, rr, 0.0f);
	v1 = Vectorize::Mul(v1, v1);
	return (v1.m128_f32[0] + v1.m128_f32[1]) <= (v1.m128_f32[2]);
#else
	return (rx * rx + ry * ry) <= (rr * rr);
#endif
}
bool DxMath::IsIntersected(DxCircle& circle, DxWidthLine& line) {
	/*
		A----B	(x1, y1)
		|    |
		|    |
		|    |
		D----C	(x2, y2)

		<---->	width
	*/

#ifdef __L_MATH_VECTORIZE
	__m128 v1, v2;
	float* _v1 = v1.m128_f32;
	float* _v2 = v2.m128_f32;

	float cen_x = (line.GetX1() + line.GetX2()) / 2.0f;
	float cen_y = (line.GetY1() + line.GetY2()) / 2.0f;

	v1 = Vectorize::Sub(
		Vectorize::Set(line.GetX2(), line.GetY2(), cen_x, cen_y),
		Vectorize::Set(line.GetX1(), line.GetY1(), circle.GetX(), circle.GetY()));

	float dx = _v1[0];
	float dy = _v1[1];
	float u_cx = _v1[2];
	float u_cy = _v1[3];

	float line_hh = Math::HypotSq(dx, dy);
	if (line_hh < FLT_EPSILON) {
		float dist = Math::HypotSq(circle.GetX() - line.GetX1(), circle.GetY() - line.GetY1());
		return dist <= (circle.GetR() * circle.GetR());
	}

	float line_h = sqrtf(line_hh);
	float rcos = dx / line_h;
	float rsin = dy / line_h;

	//Find vector cross products
	v1 = Vectorize::Mul(
		Vectorize::Set(rcos, rsin, rsin, -rcos),
		Vectorize::Set(u_cy, u_cx, u_cy, u_cx));
	float cross_x = abs(_v1[0] - _v1[1]) * 2.0f;
	float cross_y = abs(_v1[2] - _v1[3]) * 2.0f;

	bool intersect_w = cross_x <= line.GetWidth();
	bool intersect_h = cross_y <= line_h;
	if (intersect_w && intersect_h)
		return true;
	else if (intersect_w || intersect_h) {
		float r2 = circle.GetR() * 2.0f;
		if (intersect_w && (cross_y <= line_h + r2))
			return true;
		if (intersect_h && (cross_x <= line.GetWidth() + r2))
			return true;
		return false;
	}

	float l_uw = line.GetWidth() / line_h * 0.5f;
	v1 = Vectorize::Mul(
		Vectorize::Set(dx, dy, circle.GetR(), 0.0f),
		Vectorize::Set(l_uw, l_uw, circle.GetR(), 0.0f));
	float nx = _v1[0];
	float ny = _v1[1];
	float rr = _v1[2];

	//Check A and B
	v1 = Vectorize::Set(circle.GetX(), circle.GetY(), circle.GetX(), circle.GetY());
	v2 = Vectorize::Mul(v1, Vectorize::Set(line.GetX1() - ny, line.GetY1() + nx, line.GetX1() + ny, line.GetY1() - nx));
	v2 = Vectorize::Mul(v2, v2);
	if (_v2[0] + _v2[1] <= rr || _v2[2] + _v2[3] <= rr)
		return true;

	//Check C and D
	v2 = Vectorize::Sub(v1, Vectorize::Set(line.GetX2() + ny, line.GetY2() - nx, line.GetX2() - ny, line.GetY2() + nx));
	v2 = Vectorize::Mul(v2, v2);
	if (_v2[0] + _v2[1] <= rr || _v2[2] + _v2[3] <= rr)
		return true;
#else
	float rr = circle.GetR() * circle.GetR();
	float cen_x = (line.GetX1() + line.GetX2()) / 2.0f;
	float cen_y = (line.GetY1() + line.GetY2()) / 2.0f;
	float dx = line.GetX2() - line.GetX1();
	float dy = line.GetY2() - line.GetY1();
	float line_hh = Math::HypotSq(dx, dy);
	float line_h = sqrtf(line_hh);

	float rcos = dx / line_h;
	float rsin = dy / line_h;

	float cross_x = abs(rcos * (cen_y - circle.GetY()) - rsin * (cen_x - circle.GetX())) * 2.0f;
	float cross_y = abs(rsin * (cen_y - circle.GetY()) + rcos * (cen_x - circle.GetX())) * 2.0f;

	bool intersect_w = cross_x <= line.GetWidth();
	bool intersect_h = cross_y <= line_h;
	if (intersect_w && intersect_h)
		return true;
	else if (intersect_w || intersect_h) {
		float r2 = circle.GetR() * 2.0f;
		if (intersect_w && (cross_y <= line_h + r2))
			return true;
		if (intersect_h && (cross_x <= line.GetWidth() + r2))
			return true;
		return false;
	}

	float l_uw = line.GetWidth() / line_h * 0.5f;
	float nx = dx * l_uw;
	float ny = dy * l_uw;

	auto CheckDist = [&](float _tx, float _ty) -> bool {
		return Math::HypotSq(_tx - circle.GetX(), _ty - circle.GetY()) <= rr;
	};

	if (CheckDist(line.GetX1() - ny, line.GetY1() + nx)
		|| CheckDist(line.GetX1() + ny, line.GetY1() - nx)
		|| CheckDist(line.GetX2() + ny, line.GetY2() - nx)
		|| CheckDist(line.GetX2() - ny, line.GetY2() + nx))
		return true;
#endif

	return false;
}

//I want to die
bool DxMath::IsIntersected(DxWidthLine& line1, DxWidthLine& line2) {
	DxWidthLine linePairA[2];
	DxWidthLine linePairB[2];
	size_t countCheckA = DxMath::SplitWidthLine(linePairA, &line1);
	size_t countCheckB = DxMath::SplitWidthLine(linePairB, &line2);

	if (countCheckA == 0 || countCheckB == 0) 
		//This ain't a bloody point-point intersection, NEXT!!!
		return false;
	else if (countCheckA == 1 && countCheckB == 1) {
		//A normal line-line intersection
#ifdef __L_MATH_VECTORIZE
		__m128 vec1 = Vectorize::Set(linePairA->GetX2(), linePairA->GetY2(), linePairB->GetX2(), linePairB->GetY2());
		__m128 vec2 = Vectorize::Set(linePairA->GetX1(), linePairA->GetY1(), linePairB->GetX1(), linePairB->GetY1());
		float* _v1 = vec1.m128_f32;
		float* _v2 = vec2.m128_f32;

		vec1 = Vectorize::Sub(vec1, vec2);
		vec2 = Vectorize::Sub(
			Vectorize::Set(_v1[0] * _v1[3], linePairA->GetX1(), linePairA->GetY1(), 0.0f), 
			Vectorize::Set(_v1[2] * _v1[1], linePairB->GetX1(), linePairB->GetY1(), 0.0f));

		float idet = 1.0f / _v2[0];
		vec2 = Vectorize::Mul(vec1, Vectorize::Set(_v2[2], _v2[1], _v2[2], _v2[1]));
		vec2 = Vectorize::Mul(vec2, Vectorize::Replicate(idet));
		float s = _v2[0] - _v2[1];
		float t = _v2[2] - _v2[3];
#else
		float dx1 = linePairA->GetX2() - linePairA->GetX1();
		float dy1 = linePairA->GetY2() - linePairA->GetY1();
		float dx2 = linePairB->GetX2() - linePairB->GetX1();
		float dy2 = linePairB->GetY2() - linePairB->GetY1();

		float idet = 1.0f / (dx1 * dy2 - dx2 * dy1);
		float ddx = linePairA->GetX1() - linePairB->GetX1();
		float ddy = linePairA->GetY1() - linePairB->GetY1();
		float s = (dx1 * ddy - dy1 * ddx) * idet;
		float t = (dx2 * ddy - dy2 * ddx) * idet;
#endif

		//Issue: fails if the lines are intersecting but parallel
		return (s >= 0 && s <= 1 && t >= 0 && t <= 1);
	}
	else {
		//A polygon-polygon intersection then

		class PolygonIntersect {
		public:
			static const bool IntersectPolygon(DxPoint* listPointA, DxPoint* listPointB, size_t countPointA, size_t countPointB) {
				DxPoint* listPolygon[2] = { listPointA, listPointB };
				size_t listPointCount[2] = { countPointA, countPointB };

				for (size_t iPoly = 0U; iPoly < 2U; ++iPoly) {
					size_t countVert = listPointCount[iPoly];
					for (size_t iPoint = 0U; iPoint < countVert; ++iPoint) {
						DxPoint* p1 = &listPolygon[iPoly][iPoint];
						DxPoint* p2 = &listPolygon[iPoly][(iPoint + 1U) % countVert];
						DxPoint normal(p2->GetY() - p1->GetY(), p1->GetX() - p2->GetX());

						float minA = FLT_MAX;
						float maxA = FLT_MIN;
						for (size_t iPA = 0U; iPA < countPointA; ++iPA) {
							float proj = normal.GetX() * listPointA[iPA].GetX() + normal.GetY() * listPointA[iPA].GetY();
							minA = std::min(minA, proj);
							maxA = std::max(maxA, proj);
						}

						float minB = FLT_MAX;
						float maxB = FLT_MIN;
						for (size_t iPB = 0U; iPB < countPointB; ++iPB) {
							float proj = normal.GetX() * listPointB[iPB].GetX() + normal.GetY() * listPointB[iPB].GetY();
							minB = std::min(minB, proj);
							maxB = std::max(maxB, proj);
						}

						if (maxA < minB || maxB < minA) return false;
					}
				}
				return true;
			}
			static const void FillVertex(DxPoint* pVerts, DxWidthLine* pSrc, size_t countLine) {
				if (countLine == 2U) {
					pVerts[0] = DxPoint(pSrc[0].GetX1(), pSrc[0].GetY1());
					pVerts[1] = DxPoint(pSrc[1].GetX1(), pSrc[1].GetY1());
					pVerts[2] = DxPoint(pSrc[1].GetX2(), pSrc[1].GetY2());
					pVerts[3] = DxPoint(pSrc[0].GetX2(), pSrc[0].GetY2());
				}
				else {
					pVerts[0] = DxPoint(pSrc[0].GetX1(), pSrc[0].GetY1());
					pVerts[1] = DxPoint(pSrc[0].GetX2(), pSrc[0].GetY2());
				}
			};
		};

		DxPoint listPointA[4];
		DxPoint listPointB[4];
		PolygonIntersect::FillVertex(listPointA, linePairA, countCheckA);
		PolygonIntersect::FillVertex(listPointB, linePairB, countCheckB);

		return PolygonIntersect::IntersectPolygon(listPointA, listPointB, countCheckA * 2U, countCheckB * 2U);
	}

	return false;
}

bool DxMath::IsIntersected(DxLine3D& line, std::vector<DxTriangle>& triangles, std::vector<D3DXVECTOR3>& out) {
	out.clear();

	for (size_t iTri = 0; iTri < triangles.size(); ++iTri) {
		DxTriangle& tri = triangles[iTri];
		D3DXPLANE plane;//3ŠpŒ`‚Ì–Ê
		D3DXPlaneFromPoints(&plane, &tri.GetPosition(0), &tri.GetPosition(1), &tri.GetPosition(2));

		D3DXVECTOR3 vOut;// –Ê‚ÆŽ‹ü‚ÌŒð“_‚ÌÀ•W
		if (D3DXPlaneIntersectLine(&vOut, &plane, &line.GetPosition(0), &line.GetPosition(1))) {
			// “àŠO”»’è
			D3DXVECTOR3 vN[3];
			D3DXVECTOR3 vv1, vv2, vv3;
			vv1 = tri.GetPosition(0) - vOut;
			vv2 = tri.GetPosition(1) - vOut;
			vv3 = tri.GetPosition(2) - vOut;
			D3DXVec3Cross(&vN[0], &vv1, &vv3);
			D3DXVec3Cross(&vN[1], &vv2, &vv1);
			D3DXVec3Cross(&vN[2], &vv3, &vv2);
			if (D3DXVec3Dot(&vN[0], &vN[1]) < 0 || D3DXVec3Dot(&vN[0], &vN[2]) < 0)
				continue;
			else {// “à‘¤(3ŠpŒ`‚ÉÚG)
				out.push_back(vOut);
			}
		}
	}//for(int i=0;i<tris.size();i++)

	bool res = out.size() > 0;
	return res;
}

size_t DxMath::SplitWidthLine(DxWidthLine(&dest)[2], DxWidthLine* pSrcLine, float mulWidth, bool bForceDouble) {
	float dx = pSrcLine->GetX2() - pSrcLine->GetX1();
	float dy = pSrcLine->GetY2() - pSrcLine->GetY1();

	float dl = hypotf(dx, dy);
	if (dl == 0.0f) return 0U;

	float width = pSrcLine->GetWidth() * mulWidth;

	if (abs(width) <= 1.0f) {
		if (!bForceDouble) {
			dest[0] = *pSrcLine;
			return 1U;
		}
		else width = 1.0f;
	}

	__m128 vec1 = Vectorize::Set(pSrcLine->GetX1(), pSrcLine->GetY1(), pSrcLine->GetX2(), pSrcLine->GetY2());
	__m128 vec2 = Vectorize::Set(dy, -dx, dy, -dx);
	vec2 = Vectorize::Mul(vec2, Vectorize::Replicate(width / dl * 0.5f));
	{
		__m128 res = Vectorize::Add(vec1, vec2);
		vec2 = Vectorize::Mul(vec2, Vectorize::Replicate(-1.0f));
		dest[0] = DxWidthLine(res.m128_f32[0], res.m128_f32[1], res.m128_f32[2], res.m128_f32[3], 1.0f);
		res = Vectorize::Add(vec1, vec2);
		dest[1] = DxWidthLine(res.m128_f32[0], res.m128_f32[1], res.m128_f32[2], res.m128_f32[3], 1.0f);
	}

	return 2U;
};

void DxMath::ConstructRotationMatrix(D3DXMATRIX* mat, const D3DXVECTOR2& angleX, 
	const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ)
{
	float cx = angleX.x;
	float sx = angleX.y;
	float cy = angleY.x;
	float sy = angleY.y;
	float cz = angleZ.x;
	float sz = angleZ.y;
	float sx_sy = sx * sy;
	float sx_cy = sx * cy;
#ifdef __L_MATH_VECTORIZE
	__m128 v1 = Vectorize::Mul(Vectorize::Set(cy, sx_sy, cx, sy), Vectorize::Set(cz, sz, sz, cz));
	__m128 v2 = Vectorize::Mul(Vectorize::Set(sx_cy, cy, sx_sy, cx), Vectorize::Set(sz, sz, cz, cz));
	__m128 v3 = Vectorize::Mul(Vectorize::Set(sy, sx_cy, cx, cx), Vectorize::Set(sz, cz, sy, cy));
	mat->_11 = v1.m128_f32[0] - v1.m128_f32[1];
	mat->_12 = -v1.m128_f32[2];
	mat->_13 = v1.m128_f32[3] + v2.m128_f32[0];
	mat->_21 = v2.m128_f32[1] + v2.m128_f32[2];
	mat->_22 = v2.m128_f32[3];
	mat->_23 = v3.m128_f32[0] - v3.m128_f32[1];
	mat->_31 = -v3.m128_f32[2];
	mat->_32 = sx;
	mat->_33 = v3.m128_f32[3];
#else
	mat->_11 = cy * cz - sx_sy * sz;
	mat->_12 = -cx * sz;
	mat->_13 = sy * cz + sx_cy * sz;
	mat->_21 = cy * sz + sx_sy * cz;
	mat->_22 = cx * cz;
	mat->_23 = sy * sz - sx_cy * cz;
	mat->_31 = -cx * sy;
	mat->_32 = sx;
	mat->_33 = cx * cy;
#endif
}
void DxMath::MatrixApplyScaling(D3DXMATRIX* mat, const D3DXVECTOR3& scale) {
#ifdef __L_MATH_VECTORIZE
	__m256 v_mat = Vectorize::Mul(
		Vectorize::Set(mat->_11, mat->_12, mat->_13, mat->_21, mat->_22, mat->_23, mat->_31, mat->_32),
		Vectorize::Set(scale.x, scale.x, scale.x, scale.y, scale.y, scale.y, scale.z, scale.z));
	mat->_11 = v_mat.m256_f32[0];
	mat->_12 = v_mat.m256_f32[1];
	mat->_13 = v_mat.m256_f32[2];
	mat->_21 = v_mat.m256_f32[3];
	mat->_22 = v_mat.m256_f32[4];
	mat->_23 = v_mat.m256_f32[5];
	mat->_31 = v_mat.m256_f32[6];
	mat->_32 = v_mat.m256_f32[7];
	mat->_33 = mat->_33 * scale.z;
#else
	mat->_11 *= scale.x;
	mat->_12 *= scale.x;
	mat->_13 *= scale.x;
	mat->_21 *= scale.y;
	mat->_22 *= scale.y;
	mat->_23 *= scale.y;
	mat->_31 *= scale.z;
	mat->_32 *= scale.z;
	mat->_33 *= scale.z;
#endif
}
D3DXVECTOR4 DxMath::RotatePosFromXYZFactor(D3DXVECTOR4& vec, D3DXVECTOR2* angX, D3DXVECTOR2* angY, D3DXVECTOR2* angZ) {
	float vx = vec.x;
	float vy = vec.y;
	float vz = vec.z;
	float cx = angX->x;
	float sx = angX->y;
	float cy = angY->x;
	float sy = angY->y;
	float cz = angZ->x;
	float sz = angZ->y;
#ifdef __L_MATH_VECTORIZE
	if (angZ) {
		__m128 v_res = Vectorize::Mul(Vectorize::Set(vx, vy, vx, vy), Vectorize::Set(cz, sz, sz, cz));
		vec.x = v_res.m128_f32[0] - v_res.m128_f32[1];
		vec.y = v_res.m128_f32[2] + v_res.m128_f32[3];
		vx = vec.x;
		vy = vec.y;
	}
	if (angX) {
		__m128 v_res = Vectorize::Mul(Vectorize::Set(vy, vz, vy, vz), Vectorize::Set(cx, sx, sx, cx));
		vec.y = v_res.m128_f32[0] - v_res.m128_f32[1];
		vec.z = v_res.m128_f32[2] + v_res.m128_f32[3];
		vy = vec.y;
		vz = vec.z;
	}
	if (angY) {
		__m128 v_res = Vectorize::Mul(Vectorize::Set(vz, vx, vz, vx), Vectorize::Set(sy, cy, cy, sy));
		vec.x = v_res.m128_f32[0] + v_res.m128_f32[1];
		vec.z = v_res.m128_f32[2] - v_res.m128_f32[3];
	}
#else
	if (angZ) {
		vec.x = vx * cz - vy * sz;
		vec.y = vx * sz + vy * cz;
		vx = vec.x;
		vy = vec.y;
	}
	if (angX) {
		vec.y = vy * cx - vz * sx;
		vec.z = vy * sx + vz * cx;
		vy = vec.y;
		vz = vec.z;
	}
	if (angY) {
		vec.x = vz * sy + vx * cy;
		vec.z = vz * cy - vx * sy;
	}
#endif
	return vec;
}
void DxMath::TransformVertex2D(VERTEX_TLX(&vert)[4], D3DXVECTOR2* scale, D3DXVECTOR2* angle,
	D3DXVECTOR2* position, D3DXVECTOR2* textureSize) 
{
#ifdef __L_MATH_VECTORIZE
	__m256 v1;
	__m128 v2;
	float* _p1 = v1.m256_f32;
	float* _p2 = v2.m128_f32;

	//First, divide the UVs
	v1 = {
		vert[0].texcoord.x, vert[0].texcoord.y, vert[1].texcoord.x, vert[1].texcoord.y,
		vert[2].texcoord.x, vert[2].texcoord.y, vert[3].texcoord.x, vert[3].texcoord.y
	};
	v1 = Vectorize::Mul(v1, Vectorize::Set(textureSize->x, textureSize->y, textureSize->x, textureSize->y,
		textureSize->x, textureSize->y, textureSize->x, textureSize->y));
	memcpy(&(vert[0].texcoord), &_p1[0], sizeof(D3DXVECTOR2));
	memcpy(&(vert[1].texcoord), &_p1[2], sizeof(D3DXVECTOR2));
	memcpy(&(vert[2].texcoord), &_p1[4], sizeof(D3DXVECTOR2));
	memcpy(&(vert[3].texcoord), &_p1[6], sizeof(D3DXVECTOR2));

	//Then, rotate and scale (2 vertices at a time)
	for (size_t i = 0; i < 4; i += 2) {
		//Rotate
		v1 = Vectorize::Set(vert[i + 0].position.x, vert[i + 0].position.y,
			vert[i + 0].position.x, vert[i + 0].position.y,
			vert[i + 1].position.x, vert[i + 1].position.y, 
			vert[i + 1].position.x, vert[i + 1].position.y);
		v1 = Vectorize::Mul(v1, Vectorize::Set(angle->x, angle->y, angle->y, angle->x,
			angle->x, angle->y, angle->y, angle->x));

		//Scale
		/*
		v1 = (__m256&)_mm_fmadd_ps(Vectorize::Set(_p1[0] - _p1[1], _p1[2] + _p1[3], _p1[4] - _p1[5], _p1[6] + _p1[7]), 
			Vectorize::Set(scale->x, scale->y, scale->x, scale->y), 
			Vectorize::Set(position->x, position->y, position->x, position->y));
		*/
		v2 = Vectorize::MulAdd(
			Vectorize::Set(_p1[0] - _p1[1], _p1[2] + _p1[3], _p1[4] - _p1[5], _p1[6] + _p1[7]),
			Vectorize::Set(scale->x, scale->y, scale->x, scale->y),
			Vectorize::Set(position->x, position->y, position->x, position->y));

		//Save
		memcpy(&(vert[i + 0].position), &_p2[0], sizeof(D3DXVECTOR2));
		memcpy(&(vert[i + 1].position), &_p2[2], sizeof(D3DXVECTOR2));
	}
#else
	vert[0].texcoord.x *= textureSize->x;
	vert[0].texcoord.y *= textureSize->y;
	vert[1].texcoord.x *= textureSize->x;
	vert[1].texcoord.y *= textureSize->y;
	vert[2].texcoord.x *= textureSize->x;
	vert[2].texcoord.y *= textureSize->y;
	vert[3].texcoord.x *= textureSize->x;
	vert[3].texcoord.y *= textureSize->y;

	for (size_t i = 0; i < 4; ++i) {
		D3DXVECTOR4& vPos = vert[i].position;
		float px = vPos.x;
		float py = vPos.y;
		vPos.x = fmaf(px * angle->x - py * angle->y, scale->x, position->x);
		vPos.y = fmaf(px * angle->y + py * angle->x, scale->y, position->y);
	}
#endif
}