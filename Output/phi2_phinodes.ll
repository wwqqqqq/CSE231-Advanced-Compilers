; ModuleID = 'phi2.ll'
source_filename = "phi2.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define dso_local i32 @_Z7computev() #0 {
  %1 = icmp sgt i32 undef, 0
  br i1 %1, label %2, label %5

2:                                                ; preds = %0
  %3 = mul nsw i32 undef, undef
  %4 = add nsw i32 undef, undef
  br label %5

5:                                                ; preds = %2, %0
  %.01 = phi i32 [ %3, %2 ], [ undef, %0 ]
  %.0 = phi i32 [ %4, %2 ], [ undef, %0 ]
  %6 = sub nsw i32 %.01, %.0
  ret i32 %6
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #1

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 9.0.1 (branches/release_90 375507)"}
