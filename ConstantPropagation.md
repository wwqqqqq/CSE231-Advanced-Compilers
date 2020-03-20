# Constant Propagation

```
void test2() {
    x = 2;
    x = y + b;
    p = &x;
    y = *p;
    *p = b;
    *p = *p1 + *p2;
    y = add1();
}
```

```
; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z5test2v() #0 {
  store i32 2, i32* @x, align 4
  %1 = load i32, i32* @y, align 4
  %2 = load i32, i32* @b, align 4
  %3 = add nsw i32 %1, %2
  store i32 %3, i32* @x, align 4
  store i32* @x, i32** @p, align 8
  %4 = load i32*, i32** @p, align 8
  %5 = load i32, i32* %4, align 4
  store i32 %5, i32* @y, align 4
  %6 = load i32, i32* @b, align 4
  %7 = load i32*, i32** @p, align 8
  store i32 %6, i32* %7, align 4
  %8 = load i32*, i32** @p1, align 8
  %9 = load i32, i32* %8, align 4
  %10 = load i32*, i32** @p2, align 8
  %11 = load i32, i32* %10, align 4
  %12 = add nsw i32 %9, %11
  %13 = load i32*, i32** @p, align 8
  store i32 %12, i32* %13, align 4
  %14 = call i32 @_Z4add1v()
  store i32 %14, i32* @y, align 4
  ret void
}
```

```
X = N
```

```
X = Y op z
```

```
X = *Y
```

```
*X = Y
```

```
*X = *Y + *Z
```

```
X = G(...)
```
