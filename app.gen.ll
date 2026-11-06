; ModuleID = 'full'
source_filename = "full"

define void @app() {
entry:
  %regFile = alloca [16 x i32], align 4
  %0 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 1
  store i32 0, ptr %0, align 4
  br label %loop_step

loop_step:                                        ; preds = %inc_step, %entry
  %1 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 2
  store i32 0, ptr %1, align 4
  br label %loop_y

loop_y:                                           ; preds = %inc_y, %loop_step
  %2 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 5
  store i32 0, ptr %2, align 4
  %3 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 2
  %4 = load i32, ptr %3, align 4
  %5 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 1
  %6 = load i32, ptr %5, align 4
  %7 = mul i32 %4, %6
  %8 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 3
  store i32 %7, ptr %8, align 4
  br label %loop_x

loop_x:                                           ; preds = %loop_x, %loop_y
  %9 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 3
  %10 = load i32, ptr %9, align 4
  %11 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 5
  %12 = load i32, ptr %11, align 4
  %13 = mul i32 %10, %12
  %14 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 6
  store i32 %13, ptr %14, align 4
  %15 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 6
  %16 = load i32, ptr %15, align 4
  %17 = add i32 %16, -16777216
  %18 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 6
  store i32 %17, ptr %18, align 4
  %19 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 5
  %20 = load i32, ptr %19, align 4
  %21 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 2
  %22 = load i32, ptr %21, align 4
  %23 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 6
  %24 = load i32, ptr %23, align 4
  call void @simPutPixel(i32 %20, i32 %22, i32 %24)
  %25 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 5
  %26 = load i32, ptr %25, align 4
  %27 = add i32 %26, 1
  store i32 %27, ptr %25, align 4
  %28 = icmp ne i32 %27, 1536
  %29 = zext i1 %28 to i32
  %30 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 4
  store i32 %29, ptr %30, align 4
  %31 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 4
  %32 = load i32, ptr %31, align 4
  %33 = trunc i32 %32 to i1
  br i1 %33, label %loop_x, label %inc_y

inc_y:                                            ; preds = %loop_x
  %34 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 2
  %35 = load i32, ptr %34, align 4
  %36 = add i32 %35, 1
  store i32 %36, ptr %34, align 4
  %37 = icmp ne i32 %36, 768
  %38 = zext i1 %37 to i32
  %39 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 4
  store i32 %38, ptr %39, align 4
  %40 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 4
  %41 = load i32, ptr %40, align 4
  %42 = trunc i32 %41 to i1
  br i1 %42, label %loop_y, label %inc_step

inc_step:                                         ; preds = %inc_y
  call void @simFlush()
  %43 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 1
  %44 = load i32, ptr %43, align 4
  %45 = add i32 %44, 1
  store i32 %45, ptr %43, align 4
  %46 = icmp ne i32 %45, 1000
  %47 = zext i1 %46 to i32
  %48 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 4
  store i32 %47, ptr %48, align 4
  %49 = getelementptr [16 x i32], ptr %regFile, i32 0, i32 4
  %50 = load i32, ptr %49, align 4
  %51 = trunc i32 %50 to i1
  br i1 %51, label %loop_step, label %done

done:                                             ; preds = %inc_step
  ret void
}

declare void @simPutPixel(i32, i32, i32)

declare void @simFlush()
