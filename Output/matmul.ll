; ModuleID = 'matmul.cpp'
source_filename = "matmul.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32* @_Z6genMatii(i32, i32) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32*, align 8
  %6 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  store i32 %1, i32* %4, align 4
  %7 = load i32, i32* %3, align 4
  %8 = sext i32 %7 to i64
  %9 = mul i64 4, %8
  %10 = load i32, i32* %4, align 4
  %11 = sext i32 %10 to i64
  %12 = mul i64 %9, %11
  %13 = call noalias i8* @malloc(i64 %12) #3
  %14 = bitcast i8* %13 to i32*
  store i32* %14, i32** %5, align 8
  store i32 0, i32* %6, align 4
  br label %15

15:                                               ; preds = %27, %2
  %16 = load i32, i32* %6, align 4
  %17 = load i32, i32* %3, align 4
  %18 = load i32, i32* %4, align 4
  %19 = mul nsw i32 %17, %18
  %20 = icmp slt i32 %16, %19
  br i1 %20, label %21, label %30

21:                                               ; preds = %15
  %22 = load i32, i32* %6, align 4
  %23 = load i32*, i32** %5, align 8
  %24 = load i32, i32* %6, align 4
  %25 = sext i32 %24 to i64
  %26 = getelementptr inbounds i32, i32* %23, i64 %25
  store i32 %22, i32* %26, align 4
  br label %27

27:                                               ; preds = %21
  %28 = load i32, i32* %6, align 4
  %29 = add nsw i32 %28, 1
  store i32 %29, i32* %6, align 4
  br label %15

30:                                               ; preds = %15
  %31 = load i32*, i32** %5, align 8
  ret i32* %31
}

; Function Attrs: nounwind
declare dso_local noalias i8* @malloc(i64) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @_Z6matmuliiiPiS_S_(i32, i32, i32, i32*, i32*, i32*) #0 {
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32*, align 8
  %11 = alloca i32*, align 8
  %12 = alloca i32*, align 8
  %13 = alloca i32, align 4
  %14 = alloca i32, align 4
  %15 = alloca i32, align 4
  store i32 %0, i32* %7, align 4
  store i32 %1, i32* %8, align 4
  store i32 %2, i32* %9, align 4
  store i32* %3, i32** %10, align 8
  store i32* %4, i32** %11, align 8
  store i32* %5, i32** %12, align 8
  store i32 0, i32* %13, align 4
  br label %16

16:                                               ; preds = %68, %6
  %17 = load i32, i32* %13, align 4
  %18 = load i32, i32* %7, align 4
  %19 = icmp slt i32 %17, %18
  br i1 %19, label %20, label %71

20:                                               ; preds = %16
  store i32 0, i32* %14, align 4
  br label %21

21:                                               ; preds = %64, %20
  %22 = load i32, i32* %14, align 4
  %23 = load i32, i32* %8, align 4
  %24 = icmp slt i32 %22, %23
  br i1 %24, label %25, label %67

25:                                               ; preds = %21
  store i32 0, i32* %15, align 4
  br label %26

26:                                               ; preds = %60, %25
  %27 = load i32, i32* %15, align 4
  %28 = load i32, i32* %9, align 4
  %29 = icmp slt i32 %27, %28
  br i1 %29, label %30, label %63

30:                                               ; preds = %26
  %31 = load i32*, i32** %10, align 8
  %32 = load i32, i32* %13, align 4
  %33 = load i32, i32* %8, align 4
  %34 = mul nsw i32 %32, %33
  %35 = load i32, i32* %15, align 4
  %36 = add nsw i32 %34, %35
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds i32, i32* %31, i64 %37
  %39 = load i32, i32* %38, align 4
  %40 = load i32*, i32** %11, align 8
  %41 = load i32, i32* %15, align 4
  %42 = load i32, i32* %9, align 4
  %43 = mul nsw i32 %41, %42
  %44 = load i32, i32* %14, align 4
  %45 = add nsw i32 %43, %44
  %46 = sext i32 %45 to i64
  %47 = getelementptr inbounds i32, i32* %40, i64 %46
  %48 = load i32, i32* %47, align 4
  %49 = mul nsw i32 %39, %48
  %50 = load i32*, i32** %12, align 8
  %51 = load i32, i32* %13, align 4
  %52 = load i32, i32* %9, align 4
  %53 = mul nsw i32 %51, %52
  %54 = load i32, i32* %14, align 4
  %55 = add nsw i32 %53, %54
  %56 = sext i32 %55 to i64
  %57 = getelementptr inbounds i32, i32* %50, i64 %56
  %58 = load i32, i32* %57, align 4
  %59 = add nsw i32 %58, %49
  store i32 %59, i32* %57, align 4
  br label %60

60:                                               ; preds = %30
  %61 = load i32, i32* %15, align 4
  %62 = add nsw i32 %61, 1
  store i32 %62, i32* %15, align 4
  br label %26

63:                                               ; preds = %26
  br label %64

64:                                               ; preds = %63
  %65 = load i32, i32* %14, align 4
  %66 = add nsw i32 %65, 1
  store i32 %66, i32* %14, align 4
  br label %21

67:                                               ; preds = %21
  br label %68

68:                                               ; preds = %67
  %69 = load i32, i32* %13, align 4
  %70 = add nsw i32 %69, 1
  store i32 %70, i32* %13, align 4
  br label %16

71:                                               ; preds = %16
  ret void
}

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dso_local i32 @main() #2 {
  %1 = alloca i32, align 4
  %2 = alloca i32*, align 8
  %3 = alloca i32*, align 8
  %4 = alloca i32*, align 8
  store i32 0, i32* %1, align 4
  %5 = call i32* @_Z6genMatii(i32 5, i32 6)
  store i32* %5, i32** %2, align 8
  %6 = call i32* @_Z6genMatii(i32 6, i32 4)
  store i32* %6, i32** %3, align 8
  %7 = call noalias i8* @malloc(i64 80) #3
  %8 = bitcast i8* %7 to i32*
  store i32* %8, i32** %4, align 8
  %9 = load i32*, i32** %2, align 8
  %10 = load i32*, i32** %3, align 8
  %11 = load i32*, i32** %4, align 8
  call void @_Z6matmuliiiPiS_S_(i32 5, i32 6, i32 4, i32* %9, i32* %10, i32* %11)
  %12 = load i32*, i32** %2, align 8
  %13 = bitcast i32* %12 to i8*
  call void @free(i8* %13) #3
  %14 = load i32*, i32** %3, align 8
  %15 = bitcast i32* %14 to i8*
  call void @free(i8* %15) #3
  %16 = load i32*, i32** %4, align 8
  %17 = bitcast i32* %16 to i8*
  call void @free(i8* %17) #3
  ret i32 0
}

; Function Attrs: nounwind
declare dso_local void @free(i8*) #1

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { noinline norecurse nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 9.0.1 (branches/release_90 375507)"}
