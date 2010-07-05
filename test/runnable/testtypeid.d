
import std.stdio;

/******************************************************/

class ABC { }

typedef ABC DEF;

TypeInfo foo()
{
    ABC c;

    return typeid(DEF);
}

void test1()
{
    TypeInfo ti = foo();

    TypeInfo_Typedef td = cast(TypeInfo_Typedef)ti;
    assert(td);

    ti = td.base;

    TypeInfo_Class tc = cast(TypeInfo_Class)ti;
    assert(tc);

    printf("%.*s\n", tc.info.name);
    assert(tc.info.name == "testtypeid.ABC");
}

/******************************************************/

void test2()
{
    assert(typeid(int) == typeid(int));
    assert(typeid(int) != typeid(uint));
}

/******************************************************/

class FOO3 { }

FOO3 foox3;

void foo3(int x, ...)
{
    printf("%d arguments\n", _arguments.length);
    for (int i = 0; i < _arguments.length; i++)
    {	writeln(_arguments[i].toString());

	if (_arguments[i] is typeid(int))
	{
	    int j = *cast(int *)_argptr;
	    _argptr += int.sizeof;
	    printf("\t%d\n", j);
	    assert(j == 2);
	}
	else if (_arguments[i] == typeid(long))
	{
	    long j = *cast(long *)_argptr;
	    _argptr += long.sizeof;
	    printf("\t%lld\n", j);
	    assert(j == 3);
	}
	else if (_arguments[i] is typeid(double))
	{
	    double d = *cast(double *)_argptr;
	    _argptr += double.sizeof;
	    printf("\t%g\n", d);
	    assert(d == 4.5);
	}
	else if (_arguments[i] is typeid(FOO3))
	{
	    FOO3 f = *cast(FOO3*)_argptr;
	    _argptr += FOO3.sizeof;
	    printf("\t%p\n", f);
	    assert(f is foox3);
	}
	else
	    assert(0);
    }
}

void test3()
{
    FOO3 f = new FOO3();

    printf("\t%p\n", f);
    foox3 = f;

    foo3(1,2,3L,4.5,f);
    foo3(1,2,3L,4.5,f);
}

/******************************************************/

void test4()
{
	TypeInfo ti;

	ti = typeid(float[]);
	assert(!(ti is null));
	ti = typeid(double[]);
	assert(!(ti is null));
	ti = typeid(real[]);
	assert(!(ti is null));

	ti = typeid(ifloat[]);
	assert(!(ti is null));
	ti = typeid(idouble[]);
	assert(!(ti is null));
	ti = typeid(ireal[]);
	assert(!(ti is null));

	ti = typeid(cfloat[]);
	assert(!(ti is null));
	ti = typeid(cdouble[]);
	assert(!(ti is null));
	ti = typeid(creal[]);
	assert(!(ti is null));

	ti = typeid(void);
	assert(!(ti is null));
	ti = typeid(void[]);
	assert(!(ti is null));
	ti = typeid(bool[]);
	assert(!(ti is null));
}

/******************************************************/

union MyUnion5{
	int i;
	byte b;
}

typedef MyUnion5* Foo5;

void test5()
{
	TypeInfo ti = typeid(Foo5);
	assert(!(ti is null));
	writefln("%s %d %d", ti.toString(), ti.tsize, (Foo5).sizeof);
	assert(ti.tsize==(Foo5).sizeof);
	assert(ti.toString()=="testtypeid.Foo5");
}

/******************************************************/

void test6()
{
	TypeInfo ti = typeid(void*);
	assert(!(ti is null));
	assert(ti.tsize==(void*).sizeof);
	assert(ti.toString()=="void*");
}

/******************************************************/

void test7()
{
	TypeInfo ti = typeid(bool*);
	assert(!(ti is null));
	assert(ti.tsize==(bool*).sizeof);
	assert(ti.toString()=="bool*");
}

/******************************************************/

void test8()
{
	TypeInfo ti = typeid(byte*);
	assert(!(ti is null));
	assert(ti.tsize==(byte*).sizeof);
	assert(ti.toString()=="byte*");
}

/******************************************************/

void test9()
{
	TypeInfo ti = typeid(byte[]);
	assert(!(ti is null));
	assert(ti.tsize==(byte[]).sizeof);
	assert(ti.toString()=="byte[]");
}

/******************************************************/

void test10()
{
	TypeInfo ti = typeid(short*);
	assert(!(ti is null));
	assert(ti.tsize==(short*).sizeof);
	assert(ti.toString()=="short*");
}

/******************************************************/

void test11()
{
	TypeInfo ti = typeid(ushort*);
	assert(!(ti is null));
	assert(ti.tsize==(ushort*).sizeof);
	assert(ti.toString()=="ushort*");
}

/******************************************************/

void test12()
{
	TypeInfo ti = typeid(int*);
	assert(!(ti is null));
	assert(ti.tsize==(int*).sizeof);
	assert(ti.toString()=="int*");
}

/******************************************************/

void test13()
{
	TypeInfo ti = typeid(uint*);
	assert(!(ti is null));
	assert(ti.tsize==(uint*).sizeof);
	assert(ti.toString()=="uint*");
}

/******************************************************/

void test14()
{
	TypeInfo ti = typeid(ulong*);
	assert(!(ti is null));
	assert(ti.tsize==(ulong*).sizeof);
	assert(ti.toString()=="ulong*");
}

/******************************************************/

void test15()
{
	TypeInfo ti = typeid(long*);
	assert(!(ti is null));
	assert(ti.tsize==(long*).sizeof);
	assert(ti.toString()=="long*");
}

/******************************************************/

void test16()
{
	TypeInfo ti = typeid(float*);
	assert(!(ti is null));
	assert(ti.tsize==(float*).sizeof);
	assert(ti.toString()=="float*");
}

/******************************************************/

void test17()
{
	TypeInfo ti = typeid(ifloat*);
	assert(!(ti is null));
	assert(ti.tsize==(ifloat*).sizeof);
	assert(ti.toString()=="ifloat*");
}

/******************************************************/

void test18()
{
	TypeInfo ti = typeid(cfloat*);
	assert(!(ti is null));
	assert(ti.tsize==(cfloat*).sizeof);
	assert(ti.toString()=="cfloat*");
}

/******************************************************/

void test19()
{
	TypeInfo ti = typeid(double*);
	assert(!(ti is null));
	assert(ti.tsize==(double*).sizeof);
	assert(ti.toString()=="double*");
}

/******************************************************/

void test20()
{
	TypeInfo ti = typeid(idouble*);
	assert(!(ti is null));
	assert(ti.tsize==(idouble*).sizeof);
	assert(ti.toString()=="idouble*");
}

/******************************************************/

void test21()
{
	TypeInfo ti = typeid(cdouble*);
	assert(!(ti is null));
	assert(ti.tsize==(cdouble*).sizeof);
	assert(ti.toString()=="cdouble*");
}

/******************************************************/

void test22()
{
	TypeInfo ti = typeid(real*);
	assert(!(ti is null));
	assert(ti.tsize==(real*).sizeof);
	assert(ti.toString()=="real*");
}

/******************************************************/

void test23()
{
	TypeInfo ti = typeid(ireal*);
	assert(!(ti is null));
	assert(ti.tsize==(ireal*).sizeof);
	assert(ti.toString()=="ireal*");
}

/******************************************************/

void test24()
{
	TypeInfo ti = typeid(creal*);
	assert(!(ti is null));
	assert(ti.tsize==(creal*).sizeof);
	assert(ti.toString()=="creal*");
}

/******************************************************/

void test25()
{
	TypeInfo ti = typeid(char*);
	assert(!(ti is null));
	assert(ti.tsize==(char*).sizeof);
	assert(ti.toString()=="char*");
}

/******************************************************/

void test26()
{
	TypeInfo ti = typeid(wchar*);
	assert(!(ti is null));
	assert(ti.tsize==(wchar*).sizeof);
	assert(ti.toString()=="wchar*");
}

/******************************************************/

void test27()
{
	TypeInfo ti = typeid(dchar*);
	assert(!(ti is null));
	assert(ti.tsize==(dchar*).sizeof);
	assert(ti.toString()=="dchar*");
}

/******************************************************/

enum MyEnum { A, B }

void test28()
{
	TypeInfo ti = typeid(MyEnum);
	assert(!(ti is null));
	assert(ti.tsize==(MyEnum).sizeof);
	assert(ti.toString()=="testtypeid.MyEnum");
}

/******************************************************/

void test29()
{
	alias void function() func;
	TypeInfo ti = typeid(func);
	assert(ti !is null);
	assert(ti.tsize == func.sizeof);
}

/******************************************************/

void test30()
{
	alias int delegate() del;
	TypeInfo ti = typeid(del);
	assert(ti !is null);
	assert(ti.tsize == del.sizeof);
}

/******************************************************/

void test31()
{
	TypeInfo ti = typeid(void);
	assert(!(ti is null));
	assert(ti.tsize == void.sizeof);
	assert(ti.toString()=="void");
}

/******************************************************/

class Foo32 { int x = 3; }
class Bar32 { long y = 4; }

void printargs(int x, ...)
{
    printf("%d arguments\n", _arguments.length);
    for (int i = 0; i < _arguments.length; i++)
    {   writeln(_arguments[i].toString());

	if (_arguments[i] == typeid(int))
	{
	    int j = *cast(int *)_argptr;
	    _argptr += int.sizeof;
	    printf("\t%d\n", j);
	}
	else if (_arguments[i] == typeid(long))
	{
	    long j = *cast(long *)_argptr;
	    _argptr += long.sizeof;
	    printf("\t%lld\n", j);
	}
	else if (_arguments[i] == typeid(double))
	{
	    double d = *cast(double *)_argptr;
	    _argptr += double.sizeof;
	    printf("\t%g\n", d);
	}
	else if (_arguments[i] == typeid(Foo32))
	{
	    Foo32 f = *cast(Foo32*)_argptr;
	    assert(f.x == 3);
	    _argptr += Foo32.sizeof;
	    printf("\t%p\n", f);
	}
	else if (_arguments[i] == typeid(Bar32))
	{
	    Bar32 b = *cast(Bar32*)_argptr;
	    assert(b.y == 4);
	    _argptr += Bar32.sizeof;
	    printf("\t%p\n", b);
	}
	else
	    assert(0);
    }
}

void test32()
{
    Foo32 f = new Foo32();
    Bar32 b = new Bar32();

    printf("%p\n", f);
    printargs(1, 2, 3L, 4.5, f, b);
}

/******************************************************/

void test33()
{
    typedef int int_t;
//    assert(typeid(int_t).next is typeid(int));
}

/******************************************************/

void test34()
{   class C { }
    C c;
    auto a = typeid(C).info;
}

/******************************************************/

void test35()
{
    auto ti = typeid(shared(int));

    assert(cast(TypeInfo_Shared)ti);

    assert((cast(TypeInfo_Shared)ti).next == typeid(int));
}

/******************************************************/

void test36()
{
    int i;
    assert(typeid(i++) == typeid(int));
    assert(i == 1);
    assert(typeid(i + 1) == typeid(int));
}

/******************************************************/

class A37 {}
class B37 : A37 {}

void test37()
{
   auto a = new B37;
   //writeln(typeid(A));
   assert(typeid(a) == typeid(B37));
}

/******************************************************/

int main()
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    test9();
    test10();
    test11();
    test12();
    test13();
    test14();
    test15();
    test16();
    test17();
    test18();
    test19();
    test20();
    test21();
    test22();
    test23();
    test24();
    test25();
    test26();
    test27();
    test28();
    test29();
    test30();
    test31();
    test32();
    test33();
    test34();
    test35();
    test36();
    test37();

    return 0;
}
