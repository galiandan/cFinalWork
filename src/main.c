#include <stdio.h>
#include <gtk/gtk.h>
#include "data_manager.h"
#include "ui.h"

int main(int argc, char* argv[]) {
    if (!load_courses("courses.txt")) {
        init_courses();
        save_courses("courses.txt");
    }
    load_students("students.txt");
    
    printf("课程已初始化共%d门\n", MAX_COURSES);
    printf("已加载%d名学生数据\n", student_count);
    
    create_ui(argc, argv);
    
    save_courses("courses.txt");
    save_students("students.txt");
    
    return 0;
}
