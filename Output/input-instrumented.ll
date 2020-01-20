; ModuleID = '/output/input-instrumented.bc'
source_filename = "input.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@"key global" = internal constant [3 x i32] [i32 2, i32 31, i32 33]
@"value global" = internal constant [3 x i32] [i32 1, i32 3, i32 3]
@"key global.1" = internal constant [3 x i32] [i32 2, i32 32, i32 53]
@"value global.2" = internal constant [3 x i32] [i32 1, i32 2, i32 1]
@"key global.3" = internal constant [4 x i32] [i32 2, i32 13, i32 32, i32 33]
@"value global.4" = internal constant [4 x i32] [i32 1, i32 1, i32 1, i32 1]
@"key global.5" = internal constant [4 x i32] [i32 2, i32 13, i32 32, i32 33]
@"value global.6" = internal constant [4 x i32] [i32 1, i32 1, i32 1, i32 1]
@"key global.7" = internal constant [2 x i32] [i32 1, i32 32]
@"value global.8" = internal constant [2 x i32] [i32 1, i32 1]
@"key global.9" = internal constant [4 x i32] [i32 1, i32 31, i32 33, i32 56]
@"value global.10" = internal constant [4 x i32] [i32 1, i32 1, i32 1, i32 1]

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @_Z3fooi(i32) #0 {
  call void @updateInstrInfo(i32 3, i32* getelementptr inbounds ([3 x i32], [3 x i32]* @"key global", i32 0, i32 0), i32* getelementptr inbounds ([3 x i32], [3 x i32]* @"value global", i32 0, i32 0))
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  store i32 0, i32* %3, align 4
  store volatile i32 0, i32* %4, align 4
  br label %5

5:                                                ; preds = %12, %1
  call void @updateInstrInfo(i32 3, i32* getelementptr inbounds ([3 x i32], [3 x i32]* @"key global.1", i32 0, i32 0), i32* getelementptr inbounds ([3 x i32], [3 x i32]* @"value global.2", i32 0, i32 0))
  %6 = load i32, i32* %3, align 4
  %7 = load i32, i32* %2, align 4
  %8 = icmp slt i32 %6, %7
  br i1 %8, label %9, label %15

9:                                                ; preds = %5
  call void @updateInstrInfo(i32 4, i32* getelementptr inbounds ([4 x i32], [4 x i32]* @"key global.3", i32 0, i32 0), i32* getelementptr inbounds ([4 x i32], [4 x i32]* @"value global.4", i32 0, i32 0))
  %10 = load volatile i32, i32* %4, align 4
  %11 = add nsw i32 %10, 1
  store volatile i32 %11, i32* %4, align 4
  br label %12

12:                                               ; preds = %9
  call void @updateInstrInfo(i32 4, i32* getelementptr inbounds ([4 x i32], [4 x i32]* @"key global.5", i32 0, i32 0), i32* getelementptr inbounds ([4 x i32], [4 x i32]* @"value global.6", i32 0, i32 0))
  %13 = load i32, i32* %3, align 4
  %14 = add nsw i32 %13, 1
  store i32 %14, i32* %3, align 4
  br label %5

15:                                               ; preds = %5
  call void @updateInstrInfo(i32 2, i32* getelementptr inbounds ([2 x i32], [2 x i32]* @"key global.7", i32 0, i32 0), i32* getelementptr inbounds ([2 x i32], [2 x i32]* @"value global.8", i32 0, i32 0))
  call void @printOutInstrInfo()
  %16 = load volatile i32, i32* %4, align 4
  ret i32 %16
}

; Function Attrs: noinline norecurse nounwind optnone uwtable
define dso_local i32 @main() #1 {
  call void @updateInstrInfo(i32 4, i32* getelementptr inbounds ([4 x i32], [4 x i32]* @"key global.9", i32 0, i32 0), i32* getelementptr inbounds ([4 x i32], [4 x i32]* @"value global.10", i32 0, i32 0))
  call void @printOutInstrInfo()
  %1 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %2 = call i32 @_Z3fooi(i32 5)
  ret i32 0
}

declare void @updateInstrInfo(i32, i32*, i32*)

declare void @printOutInstrInfo()

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noinline norecurse nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 9.0.1 (branches/release_90 375507)"}
