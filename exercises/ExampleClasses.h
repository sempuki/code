#ifndef EXAMPLE_CLASSES_H
#define EXAMPLE_CLASSES_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// (Note the code in this class is not meant to be a example of good code!)


// Some basic types to try pooling.
typedef char			ByteType;
typedef void*			PointerType;
typedef char			FixedStringType[256];

// A basic struct
struct Point
{
	int x, y, z;
};

// A class with a virtual function table
class Base1
{
public:
	Base1()					: number(rand())	{ printf("Base1() %d = %d\n", this, number); }
	virtual ~Base1()		{ printf("~Base1() %d\n", number); }

	virtual void	Foo1()	{ printf("Base1::Foo1() %d\n", number); }
	int				GetNumber()	const { return number; }

protected:
	int				number;
};

// (Another class with a virtual function table)
class Base2
{
public:
	Base2()					: number2(rand())	{ printf("Base2() %d = %d\n", this, number2); }
	virtual ~Base2()		{ printf("~Base2() %d\n", number2); }

	virtual void	Foo2() 	{ printf("Base2::Foo2() %d\n", number2); }
	int				GetNumber() const	{ return number2; }

protected:
	int				number2;
};

// A multiply-inherited class with virtual functions and a Point class inside it.
class Derived : public Base2, public Base1
{
public:
	Derived()				:	number3(rand()) { printf("Derived() %d = %d\n", this, number3); }
	~Derived()				{ printf("~Derived() %d\n", number3); }

	virtual void	Foo1()	{ printf("Derived::Foo1() %d\n", number3); }
	virtual void	Foo2()	{ printf("Derived::Foo2() %d\n", number3); }

	int GetNumber1() const  { return number; }
	int GetNumber2() const	{ return number2; }
	int GetNumber3() const	{ return number3; }
	const Point& GetPoint() const	{ return p; }

	Point p;
	int number3;
};


class NoDefaultConstructor
{
public:
	NoDefaultConstructor( int num )	: number(num) {}

	int GetNumber() const { return number; }

private:
	int number;
};


#endif //EXAMPLE_CLASSES_H
