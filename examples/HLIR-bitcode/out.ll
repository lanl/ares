; ModuleID = '<stdin>'
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%0 = type { i32, %union.sem_t, i32, i32 }
%union.sem_t = type { i64, [24 x i8] }
%union.pthread_attr_t = type { i64, [48 x i8] }

@.str = private unnamed_addr constant [18 x i8] c"The answer is %i\0A\00", align 1

; Function Attrs: nounwind uwtable
define i32 @ack(i32 %m, i32 %n) #0 {
entry:
  %retval = alloca i32, align 4
  %m.addr = alloca i32, align 4
  %n.addr = alloca i32, align 4
  store i32 %m, i32* %m.addr, align 4
  store i32 %n, i32* %n.addr, align 4
  %0 = load i32, i32* %m.addr, align 4
  %cmp = icmp eq i32 %0, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %1 = load i32, i32* %n.addr, align 4
  %add = add nsw i32 %1, 1
  store i32 %add, i32* %retval
  br label %return

if.else:                                          ; preds = %entry
  %2 = load i32, i32* %n.addr, align 4
  %cmp1 = icmp eq i32 %2, 0
  br i1 %cmp1, label %if.then.2, label %if.else.3

if.then.2:                                        ; preds = %if.else
  %3 = load i32, i32* %m.addr, align 4
  %sub = sub nsw i32 %3, 1
  %call = call i32 @ack(i32 %sub, i32 1)
  store i32 %call, i32* %retval
  br label %return

if.else.3:                                        ; preds = %if.else
  %4 = load i32, i32* %m.addr, align 4
  %sub4 = sub nsw i32 %4, 1
  %5 = load i32, i32* %m.addr, align 4
  %6 = load i32, i32* %n.addr, align 4
  %sub5 = sub nsw i32 %6, 1
  %call6 = call i32 @ack(i32 %5, i32 %sub5)
  %call7 = call i32 @ack(i32 %sub4, i32 %call6)
  store i32 %call7, i32* %retval
  br label %return

return:                                           ; preds = %if.else.3, %if.then.2, %if.then
  %7 = load i32, i32* %retval
  ret i32 %7
}

; Function Attrs: nounwind uwtable
define i32 @main(i32 %argc, i8** %argv) #0 {
entry:
  %0 = alloca i64
  %1 = alloca %0
  %2 = getelementptr %0, %0* %1, i64 0, i32 2
  store i32 3, i32* %2
  %3 = getelementptr %0, %0* %1, i64 0, i32 3
  store i32 3, i32* %3
  %4 = bitcast %0* %1 to i8*
  %5 = call i32 @pthread_create(i64* %0, %union.pthread_attr_t* null, i8* (i8*)* @hlir.pthread.wrapped.ack, i8* %4)
  %6 = alloca i64
  %7 = alloca %0
  %8 = getelementptr %0, %0* %7, i64 0, i32 2
  store i32 4, i32* %8
  %9 = getelementptr %0, %0* %7, i64 0, i32 3
  store i32 1, i32* %9
  %10 = bitcast %0* %7 to i8*
  %11 = call i32 @pthread_create(i64* %6, %union.pthread_attr_t* null, i8* (i8*)* @hlir.pthread.wrapped.ack, i8* %10)
  %12 = alloca i64
  %13 = alloca %0
  %14 = getelementptr %0, %0* %13, i64 0, i32 2
  store i32 4, i32* %14
  %15 = getelementptr %0, %0* %13, i64 0, i32 3
  store i32 1, i32* %15
  %16 = bitcast %0* %13 to i8*
  %17 = call i32 @pthread_create(i64* %12, %union.pthread_attr_t* null, i8* (i8*)* @hlir.pthread.wrapped.ack, i8* %16)
  %18 = load i64, i64* %0
  %19 = call i32 @pthread_join(i64 %18, i8** null)
  %20 = getelementptr %0, %0* %1, i64 0, i32 0
  %21 = load i32, i32* %20
  %22 = load i64, i64* %6
  %23 = call i32 @pthread_join(i64 %22, i8** null)
  %24 = getelementptr %0, %0* %7, i64 0, i32 0
  %25 = load i32, i32* %24
  %add = add nsw i32 %21, %25
  %26 = load i64, i64* %12
  %27 = call i32 @pthread_join(i64 %26, i8** null)
  %28 = getelementptr %0, %0* %13, i64 0, i32 0
  %29 = load i32, i32* %28
  %add2 = add nsw i32 %add, %29
  %call4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([18 x i8], [18 x i8]* @.str, i32 0, i32 0), i32 %add2)
  ret i32 0
}

declare i32 @printf(i8*, ...) #1

declare i32 @pthread_create(i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*)

declare i32 @pthread_exit(i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*)

declare i32 @pthread_join(i64, i8**)

declare i32 @sem_init(%union.sem_t*, i32, i32)

declare i32 @sem_wait(%union.sem_t*)

declare i32 @sem_post(%union.sem_t*)

declare i32 @sem_destroy(%union.sem_t*)

define i8* @hlir.pthread.wrapped.ack(i8*) {
entry:
  %1 = alloca i8*
  store i8* %0, i8** %1
  %2 = load i8*, i8** %1
  %3 = bitcast i8* %2 to %0*
  %4 = getelementptr %0, %0* %3, i64 0, i32 2
  %5 = load i32, i32* %4
  %6 = getelementptr %0, %0* %3, i64 0, i32 3
  %7 = load i32, i32* %6
  %8 = call i32 @ack(i32 %5, i32 %7)
  %9 = getelementptr %0, %0* %3, i64 0, i32 0
  store i32 %8, i32* %9
  ret i8* null
}

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.8.0 (git@github.com:llvm-mirror/clang.git 7eadaa71916491bb7891d5635c72ac94956db49c) (git@github.com:llvm-mirror/llvm.git 1ab6b56fd782c73a6f864f561233284974ffda3b)"}
