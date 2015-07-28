; ModuleID = 'phifutures.c'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [22 x i8] c"X is %i on branch 1!\0A\00", align 1
@.str.1 = private unnamed_addr constant [22 x i8] c"X is %i on branch 2!\0A\00", align 1
@.str.2 = private unnamed_addr constant [19 x i8] c"The answer is %i!\0A\00", align 1

; Function Attrs: nounwind uwtable
define i32 @main(i32 %argc, i8** nocapture readnone %argv) #0 {
entry:
  %cmp = icmp slt i32 %argc, 3
  br i1 %cmp, label %if.then, label %if.else.7

if.then:                                          ; preds = %entry
  %call = call i32 @ack(i32 4, i32 1) #3, !hlir.task !2
  %call1 = call i32 @ack(i32 3, i32 4) #3, !hlir.task !2
  %cmp2 = icmp eq i32 %argc, 1
  br i1 %cmp2, label %if.then.3, label %if.else

if.then.3:                                        ; preds = %if.then
  %call4 = call i32 @ack(i32 4, i32 1) #3, !hlir.task !2
  br label %if.end

if.else:                                          ; preds = %if.then
  %call5 = call i32 @ack(i32 3, i32 4) #3, !hlir.task !2
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then.3
  %x.0 = phi i32 [ %call4, %if.then.3 ], [ %call5, %if.else ]
  %call6 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([22 x i8], [22 x i8]* @.str, i64 0, i64 0), i32 %x.0) #3
  br label %if.end.17

if.else.7:                                        ; preds = %entry
  %call8 = call i32 @ack(i32 3, i32 3) #3, !hlir.task !2
  %call9 = call i32 @ack(i32 3, i32 6) #3, !hlir.task !2
  %cmp10 = icmp eq i32 %argc, 3
  br i1 %cmp10, label %if.then.11, label %if.else.13

if.then.11:                                       ; preds = %if.else.7
  %call12 = call i32 @ack(i32 3, i32 5) #3, !hlir.task !2
  br label %if.end.15

if.else.13:                                       ; preds = %if.else.7
  %call14 = call i32 @ack(i32 3, i32 6) #3, !hlir.task !2
  br label %if.end.15

if.end.15:                                        ; preds = %if.else.13, %if.then.11
  %x.1 = phi i32 [ %call12, %if.then.11 ], [ %call14, %if.else.13 ]
  %call16 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([22 x i8], [22 x i8]* @.str.1, i64 0, i64 0), i32 %x.1) #3
  br label %if.end.17

if.end.17:                                        ; preds = %if.end.15, %if.end
  %y.0 = phi i32 [ %call, %if.end ], [ %call8, %if.end.15 ]
  %z.0 = phi i32 [ %call1, %if.end ], [ %call9, %if.end.15 ]
  %x.2 = phi i32 [ %x.0, %if.end ], [ %x.1, %if.end.15 ]
  %add = add i32 %z.0, %y.0
  %add18 = add i32 %add, %x.2
  %call19 = tail call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([19 x i8], [19 x i8]* @.str.2, i64 0, i64 0), i32 %add18) #3
  ret i32 0
}

declare i32 @ack(i32, i32) #1

; Function Attrs: nounwind
declare i32 @printf(i8* nocapture readonly, ...) #2

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.8.0 "}
!2 = !{!"launch"}