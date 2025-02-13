#pragma once
class Vector2
{
public:
	float _x;
	float _y;

	Vector2();
	Vector2(float x, float y);

	Vector2 operator+(const Vector2& other) const; // ���� ����
	Vector2 operator-(const Vector2& other) const; // ���� ����
	bool operator==(const Vector2& other) const; // ���� ��
	bool operator<(const Vector2& other) const; // ���� ��

	float Magnitude() const; // ���� ũ��
	Vector2 Normalrize() const; // ���� ����
	
	float DotProduct(const Vector2& other) const;
	float Cross(const Vector2& other) const;
};
