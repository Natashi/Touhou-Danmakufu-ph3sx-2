#include "source/GcLib/pch.h"

#include "DxUtility.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//ColorAccess
**********************************************************/
D3DCOLORVALUE ColorAccess::SetColor(D3DCOLORVALUE value, D3DCOLOR color) {
	float a = (float)GetColorA(color) / 255.0f;
	float r = (float)GetColorR(color) / 255.0f;
	float g = (float)GetColorG(color) / 255.0f;
	float b = (float)GetColorB(color) / 255.0f;
	value.r *= r; value.g *= g; value.b *= b; value.a *= a;
	return value;
}
D3DMATERIAL9 ColorAccess::SetColor(D3DMATERIAL9 mat, D3DCOLOR color) {
	float a = (float)GetColorA(color) / 255.0f;
	float r = (float)GetColorR(color) / 255.0f;
	float g = (float)GetColorG(color) / 255.0f;
	float b = (float)GetColorB(color) / 255.0f;
	mat.Diffuse.r *= r; mat.Diffuse.g *= g; mat.Diffuse.b *= b; mat.Diffuse.a *= a;
	mat.Specular.r *= r; mat.Specular.g *= g; mat.Specular.b *= b; mat.Specular.a *= a;
	mat.Ambient.r *= r; mat.Ambient.g *= g; mat.Ambient.b *= b; mat.Ambient.a *= a;
	mat.Emissive.r *= r; mat.Emissive.g *= g; mat.Emissive.b *= b; mat.Emissive.a *= a;
	return mat;
}
D3DCOLOR& ColorAccess::SetColor(D3DCOLOR& src, const D3DCOLOR& mul) {
	D3DXVECTOR4 mulFac = ToVec4(mul);
	byte na = GetColorA(src) * mulFac.x;
	byte nr = GetColorR(src) * mulFac.y;
	byte ng = GetColorG(src) * mulFac.z;
	byte nb = GetColorB(src) * mulFac.w;
	src = D3DCOLOR_ARGB(na, nr, ng, nb);
	return src;
}
D3DCOLOR& ColorAccess::ApplyAlpha(D3DCOLOR& color, float alpha) {
	byte a = ColorAccess::ClampColorRet(((color >> 24) & 0xff) * alpha);
	byte r = ColorAccess::ClampColorRet(((color >> 16) & 0xff) * alpha);
	byte g = ColorAccess::ClampColorRet(((color >> 8) & 0xff) * alpha);
	byte b = ColorAccess::ClampColorRet((color & 0xff) * alpha);
	color = D3DCOLOR_ARGB(a, r, g, b);
	return color;
}
D3DCOLOR& ColorAccess::SetColorHSV(D3DCOLOR& color, int hue, int saturation, int value) {
	int i = (int)floor(hue / 60.0f) % 6;
	float f = (float)(hue / 60.0f) - (float)floor(hue / 60.0f);

	float s = saturation / 255.0f;

	int p = (int)gstd::Math::Round(value * (1.0f - s));
	int q = (int)gstd::Math::Round(value * (1.0f - s * f));
	int t = (int)gstd::Math::Round(value * (1.0f - s * (1.0f - f)));

	int red = 0;
	int green = 0;
	int blue = 0;
	switch (i) {
	case 0: red = value;	green = t;		blue = p; break;
	case 1: red = q;		green = value;	blue = p; break;
	case 2: red = p;		green = value;	blue = t; break;
	case 3: red = p;		green = q;		blue = value; break;
	case 4: red = t;		green = p;		blue = value; break;
	case 5: red = value;	green = p;		blue = q; break;
	}

	ClampColor(red);
	ClampColor(green);
	ClampColor(blue);
	color = D3DCOLOR_RGBA(red, green, blue, color >> 24);
	return color;
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
	return (rx * rx + ry * ry) <= (rr * rr);
}
bool DxMath::IsIntersected(DxCircle& circle, DxWidthLine& line) {
	//æ’[‚à‚µ‚­‚ÍI’[‚ª‰~“à‚É‚ ‚é‚©‚ğ’²‚×‚é
	{
		float radius = circle.GetR() * circle.GetR();

		float clx1 = circle.GetX() - line.GetX1();
		float cly1 = circle.GetY() - line.GetY1();
		float clx2 = circle.GetX() - line.GetX2();
		float cly2 = circle.GetY() - line.GetY2();

		float dist1 = clx1 * clx1 + cly1 * cly1;
		float dist2 = clx2 * clx2 + cly2 * cly2;

		if ((radius >= dist1 * dist1) || (radius >= dist2 * dist2))
			return true;
	}

	//ü•ª“à‚É‰~‚ª‚ ‚é‚©‚ğ’²‚×‚é
	{
		float lx1 = line.GetX2() - line.GetX1();
		float ly1 = line.GetY2() - line.GetY1();
		float cx1 = circle.GetX() - line.GetX1();
		float cy1 = circle.GetY() - line.GetY1();
		float inner1 = lx1 * cx1 + ly1 * cy1;

		float lx2 = line.GetX1() - line.GetX2();
		float ly2 = line.GetY1() - line.GetY2();
		float cx2 = circle.GetX() - line.GetX2();
		float cy2 = circle.GetY() - line.GetY2();
		float inner2 = lx2 * cx2 + ly2 * cy2;

		if (inner1 < 0 || inner2 < 0)
			return false;
	}

	float ux1 = line.GetX2() - line.GetX1();
	float uy1 = line.GetY2() - line.GetY1();
	float px = circle.GetX() - line.GetX1();
	float py = circle.GetY() - line.GetY1();

	float u = 1.0f / hypotf(ux1, uy1);

	float ux2 = ux1 * u;
	float uy2 = uy1 * u;

	float d = px * ux2 + py * uy2;
	float qx = d * ux2;
	float qy = d * uy2;

	float rx = px - qx;
	float ry = py - qy;

	float e = rx * rx + ry * ry;
	float r = line.GetWidth() + circle.GetR();

	return (e < (r * r));
}

//I want to die
bool DxMath::IsIntersected(DxWidthLine& line1, DxWidthLine& line2) {
	auto SplitLines = [](DxWidthLine* pSrcLine, DxWidthLine* pDstLine) -> size_t {
		float dl = hypotf(pSrcLine->GetX2() - pSrcLine->GetX1(), pSrcLine->GetY2() - pSrcLine->GetY1());
		if (dl == 0.0f) return 0U;

		if (abs(pSrcLine->GetWidth()) <= 1.0) {
			*pDstLine = *pSrcLine;
			return 1U;
		}

		float sideScale = pSrcLine->GetWidth() / dl * 0.5f;
		float nx = (pSrcLine->GetX1() - (pSrcLine->GetX1() + pSrcLine->GetX2()) / 2.0f) * sideScale;
		float ny = (pSrcLine->GetY1() - (pSrcLine->GetY1() + pSrcLine->GetY2()) / 2.0f) * sideScale;

		pDstLine[0] = DxWidthLine(pSrcLine->GetX1() + ny, pSrcLine->GetY1() - nx,
			pSrcLine->GetX2() + ny, pSrcLine->GetY2() - nx, 1.0f);
		pDstLine[1] = DxWidthLine(pSrcLine->GetX1() - ny, pSrcLine->GetY1() + nx,
			pSrcLine->GetX2() - ny, pSrcLine->GetY2() + nx, 1.0f);

		return 2U;
	};

	DxWidthLine linePairA[2];
	DxWidthLine linePairB[2];
	size_t countCheckA = SplitLines(&line1, linePairA);
	size_t countCheckB = SplitLines(&line2, linePairB);

	if (countCheckA == 0 || countCheckB == 0) 
		//This ain't a bloody point-point intersection, NEXT!!!
		return false;
	else if (countCheckA == 1 && countCheckB == 1) {
		//A normal line-line intersection

		float dx1 = linePairA->GetX2() - linePairA->GetX1();
		float dy1 = linePairA->GetY2() - linePairA->GetY1();
		float dx2 = linePairB->GetX2() - linePairB->GetX1();
		float dy2 = linePairB->GetY2() - linePairB->GetY1();

		float idet = 1.0f / (dx1 * dy2 - dx2 * dy1);
		float ddx = linePairA->GetX1() - linePairB->GetX1();
		float ddy = linePairA->GetY1() - linePairB->GetY1();
		float s = (dx1 * ddy - dy1 * ddx) * idet;
		float t = (dx2 * ddy - dy2 * ddx) * idet;

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

		D3DXVECTOR3 vOut;// –Ê‚Æ‹ü‚ÌŒğ“_‚ÌÀ•W
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

D3DXVECTOR4 DxMath::RotatePosFromXYZFactor(D3DXVECTOR4& vec, D3DXVECTOR2* angX, D3DXVECTOR2* angY, D3DXVECTOR2* angZ) {
	float vx = vec.x;
	float vy = vec.y;
	float vz = vec.z;
	
	if (angZ) {
		float cz = angZ->x;
		float sz = angZ->y;

		vec.x = vx * cz - vy * sz;
		vec.y = vx * sz + vy * cz;
		vx = vec.x;
		vy = vec.y;
	}
	if (angX) {
		float cx = angX->x;
		float sx = angX->y;

		vec.y = vy * cx - vz * sx;
		vec.z = vy * sx + vz * cx;
		vy = vec.y;
		vz = vec.z;
	}
	if (angY) {
		float cy = angY->x;
		float sy = angY->y;

		vec.x = vz * sy + vx * cy;
		vec.z = vz * cy - vx * sy;
	}

	return vec;
}