#ifndef GOBJECT_H
#define GOBJECT_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// 前向声明
typedef struct GObject GObject;
typedef struct GObjectClass GObjectClass;
typedef struct GType GType;
typedef struct GSignal GSignal;
typedef struct GProperty GProperty;

// 基本类型定义
typedef uint32_t g_type_t;
typedef void (*GDestroyNotify)(void *data);
typedef void (*GCallback)(void);
typedef void (*GSignalHandler)(GObject *object, void *data);
typedef void (*GPropertyNotify)(GObject *object, const char *property_name);

// 类型系统常量
#define G_TYPE_INVALID    0
#define G_TYPE_OBJECT     1
#define G_TYPE_INTERFACE  2

// 宏定义
#define G_OBJECT(obj)           ((GObject*)(obj))
#define G_OBJECT_CLASS(klass)   ((GObjectClass*)(klass))
#define G_TYPE_CHECK_INSTANCE_TYPE(instance, type) \
    (G_OBJECT(instance)->klass->type_id == (type))

// 类型注册宏
#define G_DEFINE_TYPE(TypeName, type_name, PARENT_TYPE) \
    static void type_name##_class_init(TypeName##Class *klass); \
    static void type_name##_init(TypeName *self); \
    \
    g_type_t type_name##_get_type(void) { \
        static g_type_t type_id = 0; \
        if (!type_id) { \
            type_id = g_type_register_static(PARENT_TYPE, \
                                           #TypeName, \
                                           sizeof(TypeName), \
                                           sizeof(TypeName##Class), \
                                           (GClassInitFunc)type_name##_class_init, \
                                           (GInstanceInitFunc)type_name##_init); \
        } \
        return type_id; \
    }

// 回调函数类型
typedef void (*GClassInitFunc)(void *klass);
typedef void (*GInstanceInitFunc)(void *instance);
typedef void (*GInstanceFinalizeFunc)(GObject *object);

// 信号连接信息
typedef struct GSignalConnection {
    uint32_t id;
    GSignalHandler handler;
    void *data;
    GDestroyNotify destroy_notify;
    struct GSignalConnection *next;
} GSignalConnection;

// 属性结构
struct GProperty {
    char *name;
    g_type_t value_type;
    void *default_value;
    GPropertyNotify notify;
    struct GProperty *next;
};

// 信号结构
struct GSignal {
    char *name;
    uint32_t id;
    g_type_t owner_type;
    GSignalConnection *connections;
    struct GSignal *next;
};

// 类型信息结构
struct GType {
    g_type_t type_id;
    char *name;
    g_type_t parent_type;
    size_t instance_size;
    size_t class_size;
    GClassInitFunc class_init;
    GInstanceInitFunc instance_init;
    void *class_data;
    struct GType *next;
};

// 基础对象类结构
struct GObjectClass {
    g_type_t type_id;
    struct GObjectClass *parent_class;
    
    // 虚函数表
    void (*finalize)(GObject *object);
    void (*dispose)(GObject *object);
    void (*set_property)(GObject *object, const char *property_name, void *value);
    void* (*get_property)(GObject *object, const char *property_name);
    
    // 类的属性和信号
    GProperty *properties;
    GSignal *signals;
    
    // 内存池
    void *instance_pool;
    size_t pool_size;
    size_t instances_allocated;
};

// 基础对象结构
struct GObject {
    GObjectClass *klass;
    uint32_t ref_count;
    uint32_t object_id;
    
    // 对象属性存储
    void *property_data;
    
    // 信号连接
    GSignalConnection *signal_connections;
    
    // 用户数据
    void *user_data;
    GDestroyNotify user_data_destroy;
};

// 全局类型系统
typedef struct GTypeSystem {
    GType *types;
    g_type_t next_type_id;
    uint32_t next_object_id;
    uint32_t next_signal_id;
} GTypeSystem;

// 全局函数声明

// 类型系统
void g_type_system_init(void);
void g_type_system_cleanup(void);
g_type_t g_type_register_static(g_type_t parent_type, 
                                const char *type_name,
                                size_t instance_size,
                                size_t class_size,
                                GClassInitFunc class_init,
                                GInstanceInitFunc instance_init);
GType* g_type_get_info(g_type_t type_id);
const char* g_type_get_name(g_type_t type_id);
bool g_type_is_a(g_type_t type, g_type_t is_a_type);

// 对象管理
GObject* g_object_new(g_type_t type);
GObject* g_object_ref(GObject *object);
void g_object_unref(GObject *object);
void g_object_set_data(GObject *object, void *data, GDestroyNotify destroy);
void* g_object_get_data(GObject *object);

// 属性系统
void g_object_class_install_property(GObjectClass *klass,
                                   const char *property_name,
                                   g_type_t value_type,
                                   void *default_value);
void g_object_set_property(GObject *object, const char *property_name, void *value);
void* g_object_get_property(GObject *object, const char *property_name);
void g_object_notify(GObject *object, const char *property_name);

// 信号系统
uint32_t g_signal_new(const char *signal_name, g_type_t owner_type);
uint32_t g_signal_connect(GObject *object, const char *signal_name,
                         GSignalHandler handler, void *data);
void g_signal_disconnect(GObject *object, uint32_t connection_id);
void g_signal_emit(GObject *object, const char *signal_name, ...);

// 基础对象类初始化
void g_object_class_init(GObjectClass *klass);
void g_object_init(GObject *object);

// 内存管理辅助
void* g_malloc(size_t size);
void* g_malloc0(size_t size);
void g_free(void *ptr);
char* g_strdup(const char *str);

#endif // GOBJECT_H


