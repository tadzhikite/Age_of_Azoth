#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <unistd.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <array>
#include <algorithm>
#include <set>

#include <stdio.h>
#include <fcntl.h>
#include <libevdev-1.0/libevdev/libevdev.h>

#include <sys/socket.h>
#include <netinet/in.h>
namespace A{
    struct Scalar { float val; 
        const Scalar operator+(const Scalar a)const {return Scalar{val+a.val};}
        const Scalar operator-(const Scalar a)const {return Scalar{val-a.val};}
        const Scalar operator-()const {return Scalar{-val};}
        const Scalar operator+=(const Scalar a){ val = val+a.val; return Scalar{val};}
        const Scalar operator*(const Scalar a)const { return Scalar{val*a.val};}
        const Scalar operator*(const float a)const { return Scalar{val*a};}
        const Scalar operator/(const Scalar a)const { return Scalar{val/a.val};}
        const bool operator<(const Scalar a)const { return val<a.val;}
    };
    struct R201{ 
        std::array<float, 8> val;  //1 e0 e1 e2 e01 e02 e12 e012
        const R201 operator*(const R201& a){
            return R201{};
        }
    };
    struct Line {
        std::array<Scalar, 3> val; // e0, e1, e2
        const Scalar e0()const {return val[0];}
        const Scalar e1()const {return val[1];}
        const Scalar e2()const {return val[2];}
    };
    struct Point{
        std::array<Scalar, 3> val; //e01, e20, e12
    };
    const Point wedge(const Line& a, const Line& b) {
        return Point{
            .val={
                Scalar{(a.e0()*b.e1()) - (a.e1()*b.e0())},//e01
                Scalar{(a.e2()*b.e0()) - (a.e0()*b.e2())},      //e20
                {(a.e1()*b.e2()) - (a.e2()*b.e1())},       //e12
            }};
    }
    const Scalar dot(const Point a, const Point b){
        return Scalar{((a.val[0]*b.val[0])*0.f)+(a.val[1]*b.val[1])+(a.val[2]*b.val[2])};
    }
    struct Matrix{
        std::array<Point, 3> val;
        const Matrix Transpose(){
            return Matrix{
                .val = std::array<Point, 3>{
                    Point{val[0].val[0], val[1].val[0], val[2].val[0]},
                        Point{val[0].val[1], val[1].val[1], val[2].val[1]},
                        Point{val[0].val[2], val[1].val[2], val[2].val[2]},
                }
            };
        };
        const Point operator*(Point a){
            Matrix mPrime = Matrix{val}.Transpose();
            return Point{
                dot(mPrime.val[0], a),
                    dot(mPrime.val[1], a),
                    dot(mPrime.val[2], a)
            };
        };
    };
    Matrix Projection(const Scalar E, const Scalar F){
        return Matrix{
            .val={
                Point{Scalar{1.f}, {0.f}, E},
                Point{Scalar{0.f}, {1.f}, F},
                Point{Scalar{0.f}, {0.f}, {1.f}},
            }
        };
    }
    R201 meet(const R201& a, const R201& b){
        return a;
    }
    R201 join(const R201& a, const R201& b){
        return a;
    };
    R201 dot(const R201& a, const R201& b){ return a; }
    R201 selectGrade(R201 v, int n){ R201 res; if(n==1){ res.val[0] = v.val[0]; }return res; };
    const Scalar dot(const Line a, const Line b){ return Scalar{ (a.val[0]*b.val[0])+(a.val[1]*b.val[1])}; }
    const Scalar distance(const Point& A, const Point& B){
        float ax = A.val[1].val/A.val[0].val;
        float ay = A.val[2].val/A.val[0].val;
        float bx = B.val[1].val/A.val[0].val;
        float by = B.val[2].val/A.val[0].val;
        float xd = ax-bx;
        float yd = ay-by;
        return Scalar{.val = (float)sqrt((xd*xd)+(yd*yd)) };
    }
    R201 r201(Scalar a){ return R201{.val= { a.val, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f} }; };
    xcb_segment_t xcbSegment(const Point a, const Point b){
        return xcb_segment_t{
            .x1=static_cast<int16_t>((a.val[1]/a.val[2]).val),
                .y1=static_cast<int16_t>((a.val[0]/a.val[2]).val),
                .x2=static_cast<int16_t>((b.val[1]/b.val[2]).val),
                .y2=static_cast<int16_t>((b.val[0]/b.val[2]).val),
        };
    }
}
namespace B{
    struct Scalar { 
        float val;
        Scalar() = default;
        Scalar(float a):val{a}{}
        constexpr static unsigned int grade = 0;
        const Scalar operator+(const Scalar a)const {return Scalar{val+a.val};}
        const Scalar operator-(const Scalar a)const {return Scalar{val-a.val};}
        const Scalar operator-()const {return Scalar{-val};}
        const Scalar operator+=(const Scalar a){ val = val+a.val; return Scalar{val};}
        const Scalar operator-=(const Scalar a){ val = val-a.val; return Scalar{val};}
        const Scalar operator*(const Scalar a)const { return Scalar{val*a.val};}
        const Scalar operator*(const float a)const { return Scalar{val*a};}
        const Scalar operator/(const Scalar a)const { return Scalar{val/a.val};}
        const bool operator<(const Scalar a)const { return val<a.val;}
    };
    struct R301{
        std::array<Scalar, 16> e;
    };
    struct Plane{
        constexpr static unsigned int grade = 1;
        std::array<Scalar, 4> dat;//e0, e1, e2, e3
        const Scalar e0()const {return dat[0];}
        const Scalar e1()const {return dat[1];}
        const Scalar e2()const {return dat[2];}
        const Scalar e3()const {return dat[2];}
    };
    struct Line{
        constexpr static unsigned int grade = 2;
        std::array<Scalar, 6> dat; //e01, e02, e03, e12, e31, e23
    };
    const R301 operator*(const R301 a, const R301 b){
        return R301{
            .e=std::array<Scalar, 16>{
                (a.e[0]*b.e[0])+(a.e[2]*b.e[2])+(a.e[3]*b.e[3])+(a.e[4]*b.e[4])-
                    ((a.e[8]*b.e[8])+ (a.e[9]*b.e[9])+(a.e[10]*b.e[10]))
            }
        };
    };
    struct Point{
        constexpr static unsigned int grade = 3;
        std::array<Scalar, 4> dat; //e021,e013,e032,e123
        const std::array<float, 3> xyz () const { return {(dat[0]/dat[3]).val, (dat[1]/dat[3]).val,(dat[2]/dat[3]).val}; }
    };
    struct Translator{
        std::array<Scalar, 4> val; // 1, e01, e02, e03
    };
    struct Rotor{
        std::array<Scalar, 4> val; // 1, e12, e31, e23 / Q: Quaternion / rotor
    };
    struct Motor{
        std::array<Scalar, 8> dat; //1, e01,e02,e03,e12,e31,e23, I / D: Dual quaternion / motor
    };
    Rotor XRotor(Scalar theta){
        double dt = (double)theta.val;
        return Rotor{cos(dt), 0.f, 0.f, -sin(dt)};
    };
    const R301 operator*(const Translator t, const Point p){
        //t: // 1,e01,e02,e03
        //p: // e021,e013,e032,e123
        return R301{
            //1,e0,e1,e2,e3,e01,e02,e03,e12,e31,e23,e021,e031,e032,e123,I
            
        };
    };
    const Line wedge(const Plane& a, const Plane& b){
        return Line{};
    }
    const Point wedge(const Plane& a, const Line& l){
        return Point{};
    }
    const Point wedge(const Line& a, const Plane& l){
        return Point{};
    }
    std::array<Plane, 8> Tetrahedron;
    xcb_segment_t xcbSegment(const B::Point a, const B::Point b){
        return xcb_segment_t{
            .x1 = static_cast<int16_t>( 1920.f*((a.dat[2]/a.dat[3]).val+1.f)/2.f ),
                .y1 = static_cast<int16_t>(1080.f*((a.dat[1]/a.dat[3]).val+1.f)/2.f ),
                .x2 = static_cast<int16_t>(1920.f*((b.dat[2]/b.dat[3]).val+1.f)/2.f ),
                .y2 = static_cast<int16_t>( 1080.f*((b.dat[1]/b.dat[3]).val+1.f)/2.f ),
        };
    }
}
class Application {
    const std::vector<const char*> availableValidationLayers = {
        "VK_LAYER_VALVE_steam_fossilize_64",
        "VK_LAYER_VALVE_steam_overlay_32",
        "VK_LAYER_VALVE_steam_fossilize_32",
        "VK_LAYER_VALVE_steam_overlay_64",
        "VK_LAYER_NV_optimus",
        "VK_LAYER_KHRONOS_validation"};
    const std::string ApplicationName = "Hello Triangle";
    const std::string EngineName = "No Engine";
    const std::vector<const char*> validationLayers={"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> noLayers = {};
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    std::vector<const char*> instanceExtension{
        VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_XCB_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
    struct Instance{
        VkInstance obj;
        VkApplicationInfo applicationInfo;
        VkInstanceCreateInfo instanceCreateInfo;
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        VkDebugUtilsMessengerEXT debugMessenger;
    };
    Instance createInstance(std::vector<const char*> layers) {
        Instance instance{};
        bool debug = layers.size()>=1;
        auto checkValidationLayerSupport = [](std::vector<const char*>& layers) {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
            for (const char* layerName : layers) {
                bool layerFound = false;
                for (const auto& layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }
                if (!layerFound) {
                    return false;
                }
            }
            return true;
        };
        instance.applicationInfo = VkApplicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                .pApplicationName = ApplicationName.c_str(),
                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                .pEngineName = EngineName.c_str(),
                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                .apiVersion = VK_API_VERSION_1_0,
        };
        instance.debugCreateInfo = VkDebugUtilsMessengerCreateInfoEXT {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = debugCallback
        };
        if (!checkValidationLayerSupport(layers)) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        instance.instanceCreateInfo= VkInstanceCreateInfo{
            .sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .pNext=debug?(VkDebugUtilsMessengerCreateInfoEXT*) &instance.debugCreateInfo:nullptr,
                .pApplicationInfo = &instance.applicationInfo,
                .enabledLayerCount=debug?static_cast<uint32_t>(layers.size()):0,
                .ppEnabledLayerNames=layers.data(),
                .enabledExtensionCount=static_cast<uint32_t>(instanceExtension.size()-(debug?0:1)),
                .ppEnabledExtensionNames = instanceExtension.data(),
        };
        if (vkCreateInstance(&instance.instanceCreateInfo, nullptr, &instance.obj) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        if (debug&&CreateDebugUtilsMessengerEXT(instance.obj, &instance.debugCreateInfo, nullptr, &instance.debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
        return instance;
    }
    public:
    struct Surface{
        VkSurfaceKHR obj;
        VkXcbSurfaceCreateInfoKHR info;
        xcb_screen_t* screen;
        xcb_gcontext_t xcbContext;
        xcb_pixmap_t xcbBuffer;
        xcb_font_t font;
    };
    Surface surface;
    void createSurface(std::array<uint16_t, 2> initialXYDimension, const std::string& title) {
        surface.info = VkXcbSurfaceCreateInfoKHR{
            .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
                .pNext=nullptr,
                .flags=0};
        surface.info.connection = xcb_connect (nullptr, nullptr);
        surface.screen = xcb_setup_roots_iterator(xcb_get_setup(surface.info.connection)).data;
        surface.info.window = xcb_generate_id(surface.info.connection);
        uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        uint32_t values[2] = { surface.screen->black_pixel, 
            XCB_EVENT_MASK_EXPOSURE | 
                XCB_EVENT_MASK_KEY_PRESS |
                XCB_EVENT_MASK_KEY_RELEASE |
                XCB_EVENT_MASK_STRUCTURE_NOTIFY |
                XCB_EVENT_MASK_POINTER_MOTION |  // motion with no mouse button held
                XCB_EVENT_MASK_BUTTON_PRESS   |
                XCB_EVENT_MASK_BUTTON_RELEASE |
                XCB_EVENT_MASK_BUTTON_MOTION |   // motion with one or more mouse buttons held
                XCB_EVENT_MASK_BUTTON_1_MOTION | // motion while only 1st mouse button is held
                XCB_EVENT_MASK_BUTTON_2_MOTION | // and so on...
                XCB_EVENT_MASK_BUTTON_3_MOTION |
                XCB_EVENT_MASK_BUTTON_4_MOTION |
                XCB_EVENT_MASK_BUTTON_5_MOTION
        };
        xcb_create_window (surface.info.connection, XCB_COPY_FROM_PARENT, surface.info.window, surface.screen->root,
                0, 0, initialXYDimension[0], initialXYDimension[1], 0,
                XCB_WINDOW_CLASS_INPUT_OUTPUT,
                surface.screen->root_visual, mask, values);
        xcb_change_property(surface.info.connection, XCB_PROP_MODE_REPLACE, surface.info.window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                title.size(), title.c_str());
        xcb_map_window(surface.info.connection, surface.info.window);
        xcb_flush(surface.info.connection);
        if(vkCreateXcbSurfaceKHR(instance.obj, &surface.info, nullptr,  &surface.obj)!=VK_SUCCESS){std::cout << "fail\n\n";}
        surface.xcbContext = xcb_generate_id(surface.info.connection);
        mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
        uint32_t graphicsValues[2] = { surface.screen->white_pixel, surface.screen->black_pixel };
        xcb_create_gc(surface.info.connection, surface.xcbContext, surface.info.window, mask, graphicsValues);
        surface.xcbBuffer = xcb_generate_id(surface.info.connection);
        xcb_create_pixmap(surface.info.connection, surface.screen->root_depth, surface.xcbBuffer, surface.info.window, 1980, 1080);
        surface.font = xcb_generate_id(surface.info.connection);
        xcb_open_font(surface.info.connection, surface.font, strlen("6x13"), "6x13");
    };
    struct Physical{
        VkPhysicalDevice device;
        VkPhysicalDeviceProperties properties;
        std::vector<VkQueueFamilyProperties> queueFamilies;
        std::vector<std::array<VkBool32, 4>> GCTPsupport;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    };
    void PhysicalDeviceList() {
        uint32_t deviceCount =0;
        vkEnumeratePhysicalDevices(instance.obj, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        physical.push_back({});
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance.obj, &deviceCount, physicalDevices.data());
        for(uint32_t i = 0; i < deviceCount; i++){
            physical[i].device = physicalDevices[i];
            vkGetPhysicalDeviceProperties(physical[i].device, &physical[i].properties);
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physical[i].device, &queueFamilyCount, nullptr);
            physical[i].queueFamilies.resize(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physical[i].device, &queueFamilyCount, physical[i].queueFamilies.data());
            physical[i].GCTPsupport = [this, &i](std::vector<VkQueueFamilyProperties>& queueFamily) {
                size_t famCount = physical[i].queueFamilies.size();
                auto res = std::vector<std::array<VkBool32, 4>>(famCount);
                for(size_t queueFamilyIndex=0; queueFamilyIndex < famCount; queueFamilyIndex++){
                    int j = queueFamilyIndex;
                    res[j][0]=physical[i].queueFamilies[j].queueFlags&VK_QUEUE_GRAPHICS_BIT;
                    res[j][1]=physical[i].queueFamilies[j].queueFlags&VK_QUEUE_COMPUTE_BIT;
                    res[j][2]=physical[i].queueFamilies[j].queueFlags&VK_QUEUE_TRANSFER_BIT;
                    res[j][3]=vkGetPhysicalDeviceXcbPresentationSupportKHR(
                            physical[i].device, queueFamilyIndex, 
                            surface.info.connection, surface.screen->root_visual);
                    std::cout<<"Family "<<queueFamilyIndex<<" GCTS support: ";
                    for(auto s:res[j]){
                        std::cout << (s?"+":"-");
                    }
                    std::cout << "\n";
                }
                return res;
            }(physical[i].queueFamilies);
        }
    }
    struct Logical {
        VkDevice device;
        std::vector<VkQueue> gctsQueues;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        VkDeviceCreateInfo info;
        VkPhysicalDeviceFeatures features;
    };
    Logical createLogicalDevice(Physical& gpu, const std::vector<const char*>& layers, const std::vector<const char*>& extensions) {
        std::vector<float> queuePriority(16,1.0f);
        auto& GCTPsupport = gpu.GCTPsupport;
        uint32_t queueFamilyIndex = std::find_if(GCTPsupport.begin(), GCTPsupport.end(), [](auto& a){return true;})-GCTPsupport.begin();
        Logical res{};
        res.queueCreateInfos.push_back(VkDeviceQueueCreateInfo {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamilyIndex,
                .queueCount = 16,
                .pQueuePriorities = &queuePriority[0]
                });
        res.features.samplerAnisotropy = VK_TRUE;
        res.info = VkDeviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = static_cast<uint32_t>(res.queueCreateInfos.size()),
                .pQueueCreateInfos = res.queueCreateInfos.data(),
                .enabledLayerCount = static_cast<uint32_t>(layers.size()),
                .ppEnabledLayerNames = layers.data(),
                .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
                .ppEnabledExtensionNames = deviceExtensions.data(),
                .pEnabledFeatures = &res.features,
        };
        VkResult result = vkCreateDevice(gpu.device, &res.info, nullptr, &res.device);
        if(result != VK_SUCCESS){ throw std::runtime_error("failed creating logical device!"); }
        res.gctsQueues.resize(16);
        for(int i = 0; i < 16; i++){ vkGetDeviceQueue(res.device, 0, i, &res.gctsQueues[i]); }
        return res;
    }
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }
    VkExtent2D chooseSwapExtent(std::array<uint16_t, 2> windowDimensions, const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != 0xFFFFFFFF) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(windowDimensions[0]),
                static_cast<uint32_t>(windowDimensions[0])
            };
            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
            return actualExtent;
        }
    }
    struct SwapChainSupportDetails {
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
        VkSurfaceCapabilitiesKHR capabilities;
    };
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice& gpu) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface.obj, &details.capabilities);
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface.obj, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface.obj, &formatCount, details.formats.data());
        }
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface.obj, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface.obj, &presentModeCount, details.presentModes.data());
        }
        return details;
    }
    struct Swapchain{
        VkSwapchainKHR obj;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;
        SwapChainSupportDetails swapChainSupport;
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
        VkExtent2D extent;
        std::vector<uint32_t> queueFamilyIndices;
    };
    Swapchain createSwapchain(Physical& gpu, Surface& surface, std::array<uint16_t, 2> windowDimension) {
        Swapchain swap;
        swap.swapChainSupport = querySwapChainSupport(gpu.device);
        swap.surfaceFormat = chooseSwapSurfaceFormat(swap.swapChainSupport.formats);
        swap.swapChainImageFormat = swap.surfaceFormat.format;
        swap.presentMode = chooseSwapPresentMode(swap.swapChainSupport.presentModes);
        swap.extent = chooseSwapExtent(windowDimension, swap.swapChainSupport.capabilities);
        swap.queueFamilyIndices = {0};
        uint32_t imageCount = swap.swapChainSupport.capabilities.minImageCount + 1;
        if (swap.swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swap.swapChainSupport.capabilities.maxImageCount) {
            imageCount = swap.swapChainSupport.capabilities.maxImageCount;
        }
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface.obj;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = swap.surfaceFormat.format;
        createInfo.imageColorSpace = swap.surfaceFormat.colorSpace;
        createInfo.imageExtent = swap.extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (swap.queueFamilyIndices.size()>1) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 1;
            createInfo.pQueueFamilyIndices = swap.queueFamilyIndices.data();
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        createInfo.preTransform = swap.swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = swap.presentMode;
        createInfo.clipped = VK_TRUE;
        if (vkCreateSwapchainKHR(logical.device, &createInfo, nullptr, &swap.obj) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }
        vkGetSwapchainImagesKHR(logical.device, swap.obj, &imageCount, nullptr);
        swap.swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(logical.device, swap.obj, &imageCount, swap.swapChainImages.data());
        return swap;
    }
    struct Input{
        float timeStamp;
        bool shouldResize;
        std::array<uint16_t, 2> dimension;
        std::array<int16_t, 2> bearing;
        std::array<int16_t, 2> resize;
        std::array<int16_t, 2> pointer;
        std::array<int16_t, 2> pointerStake;
        std::array<int, 5> buttons;
        int button;
        std::string msg;
    };
    std::array<Input, 2> poll(std::array<Input, 2>& old) {
        xcb_generic_event_t *event;
        std::array<Input, 2> res{};
        std::transform(old.begin(), old.end()-1, res.begin()+1, [](Input& i){ return i; });
        res[0] = res[1];
        res[0].button = (res[1].button==0)?1:res[0].button;
        res[0].button = (res[1].button==3)?2:res[0].button;
        for(size_t i = 0; i < res[0].buttons.size();i++){
            res[0].buttons[i] = (res[1].buttons[i]==0)?1:res[0].buttons[i];
            res[0].buttons[i] = (res[1].buttons[i]==3)?2:res[0].buttons[i];
        }
        while ((event = xcb_poll_for_event(surface.info.connection))) {
            switch (event->response_type & ~0x80) {
                case XCB_CONFIGURE_NOTIFY: {
                                               xcb_configure_notify_event_t* cfgEvent = (xcb_configure_notify_event_t*)event;
                                               //handleResize(cfgEvent->width, cfgEvent->height);
                                               res[0].dimension =
                                                   std::array<uint16_t, 2>{cfgEvent->width, cfgEvent->height};
                                               res[0].bearing =
                                                   std::array<int16_t, 2>{cfgEvent->x, cfgEvent->y};
                                               res[0].resize =
                                                   std::array<int16_t, 2>{static_cast<int16_t>(cfgEvent->x-static_cast<int16_t>(res[0].dimension[0])), 
                                                       static_cast<int16_t>(cfgEvent->y-static_cast<int16_t>(res[0].dimension[1]))};

                                               break;
                                           }
                case XCB_EXPOSE:
                                           break;
                case XCB_KEY_PRESS:
                                           std::cout<<"key\n";
                                           break;
                case XCB_KEY_RELEASE: 
                                           std::cout << "key release\n";
                                           break;
                case XCB_BUTTON_PRESS: {
                                           xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
                                           res[0].button = 3;
                                           res[0].buttons[ev->detail-1] = 3;
                                           if(ev->detail==1){
                                               res[0].pointerStake = std::array<int16_t,2>{ev->event_x, ev->event_y};
                                               res[0].pointer = std::array<int16_t,2>{ev->event_x, ev->event_y};
                                           }
                                           break;
                                       }
                case XCB_BUTTON_RELEASE: {
                                             xcb_button_release_event_t *ev = (xcb_button_release_event_t *)event;
                                             res[0].button = 0;
                                             res[0].buttons[ev->detail-1] = 0;
                                             if(ev->detail==4||ev->detail==5){
                                                 res[0].buttons[ev->detail-1] = 3;
                                             }
                                             break;
                                         }
                case XCB_MOTION_NOTIFY:   {
                                              xcb_motion_notify_event_t *enter = (xcb_motion_notify_event_t *)event;
                                              //printf ("Mouse moved in window %" PRIu32", at coordinates (%" PRIi16",%" PRIi16")\n",
                                              //        enter->event, enter->event_x, enter->event_y );
                                              res[0].pointer = std::array<int16_t, 2>{
                                                  enter->event_x,
                                                      enter->event_y
                                              };
                                              break;
                                          }
            }
            free(event);
        }
        return res;
    }
    struct Ellipse{
        float width=300.f, height=120.f;
        float a = 40.f,  b = 120.f;
        float theta = 0.f;
        float x(float u){
            return a*(1.f-(u*u))/(1.f+(u*u)); 
        }
        float y(float u){
            return b*(2.f*u)/(1+(u*u)); 
        }
        std::vector<xcb_point_t> EllipsePoints(std::array<float, 2> f1, std::array<float,2>f2, float ecc){ 
            std::array<float, 2> origin = {
                (f1[0]+f2[0])/2.f,
                (f1[1]+f2[1])/2.f
            };
            std::array<float,2> fv = {(f2[0]-origin[0]), f2[1]-origin[1]};
            float c = sqrt(fv[0]*fv[0]+fv[1]*fv[1]);
            float a = c/ecc;
            float b=sqrt(a*a-c*c);
            float phi = atan2(fv[1],fv[0]);
            std::vector<float> t(61);
            std::generate(t.begin(), t.end(), [f=0.f]()mutable{f+= (2.f*3.14f)/60.f; return f;});
            std::vector<xcb_point_t> res;
            std::transform(t.begin(), t.end(), std::back_inserter(res), [&a, &b, &phi, &origin](float t){
                    std::array<float, 2> v1 = {
                    static_cast<float>(a*cos(t)),
                    static_cast<float>(b*sin(t)),
                    };
                    std::array<float, 2> v = {
                    static_cast<float>(v1[0]*cos(phi)-v1[1]*sin(phi)),
                    static_cast<float>(v1[0]*sin(phi)+v1[1]*cos(phi)),
                    };
                    return xcb_point_t{static_cast<int16_t>(v[0]+origin[0]),static_cast<int16_t>(v[1]+origin[1])};
                    });
            return res; 
        }
        std::vector<xcb_point_t> GridPoints(std::array<float, 2> f1, std::array<float,2> f2, float ab, float N){ 
            std::array<float,2> origin={
                (f1[0]+f2[0])/2.f,
                (f1[1]+f2[1])/2.f
            };
            std::array<float, 2> cv = {f2[0]-origin[0], f2[1]-origin[1]};
            float c = sqrt(cv[0]*cv[0]+cv[1]*cv[1]);
            float a = ab*c;
            float b = a/ab;
            float phi=0.f;

            std::vector<xcb_point_t> res;
            for(float i = 0.f; i < N; i+=1.f){
                std::array<float, 2> f1= {
                    -c,
                    0.f,
                };
                std::array<float, 2> f2= {
                    c,
                    0.f,
                };
                std::array<float, 2> p= {
                    a*(float)cos(2*3.14159f*i/N),
                    b*(float)sin(2*3.14159f*i/N)
                };
                std::array<float, 2> vr= {
                    static_cast<float>(f1[0]*cos(phi)-f1[1]*sin(phi))+origin[0],
                    static_cast<float>(f1[0]*sin(phi)+f1[1]*cos(phi))+origin[1],
                };
                std::array<float, 2> vr1 {
                    static_cast<float>(p[0]*cos(phi)-p[1]*sin(phi))+origin[0],
                    static_cast<float>(p[0]*sin(phi)+p[1]*cos(phi))+origin[1],
                };
                std::array<float, 2> vr2= {
                    static_cast<float>(f2[0]*cos(phi)-f2[1]*sin(phi))+origin[0],
                    static_cast<float>(f2[0]*sin(phi)+f2[1]*cos(phi))+origin[1],
                };
                res.push_back({
                        static_cast<int16_t>(vr[0]), 
                        static_cast<int16_t>(vr[1])}
                        );
                res.push_back({
                        static_cast<int16_t>(vr1[0]), 
                        static_cast<int16_t>(vr1[1])}
                        );
                res.push_back({
                        static_cast<int16_t>(vr2[0]), 
                        static_cast<int16_t>(vr2[1])}
                        );
            }
            return res; 
        }
        void draw(std::array<float,2> f1, std::array<float,2> f2, float ecc, uint32_t color, Surface& surface, xcb_pixmap_t& back_buffer){
            uint32_t    mask = XCB_GC_FOREGROUND;
            auto points = EllipsePoints(f1,f2,ecc);
            xcb_change_gc(surface.info.connection, surface.xcbContext, mask, &color);
            xcb_poly_line(surface.info.connection, XCB_COORD_MODE_ORIGIN, back_buffer, surface.xcbContext,points.size(), &points[0]);
            //points = GridPoints(f1,f2, ecc,N);
            //xcb_poly_line(surface.info.connection, XCB_COORD_MODE_ORIGIN, back_buffer, surface.xcbContext,points.size(), &points[0]);
        }
    };
    struct Grid{
        std::vector<xcb_segment_t> GridPoints(float N, float M){ 
            std::vector<xcb_segment_t> res;
            for(A::Scalar i = A::Scalar{0.f}; i < A::Scalar{1920.f}; i+=(A::Scalar{1.f/16.f}*A::Scalar{1920.f})){
                A::Line top = A::Line{A::Scalar{0.f}, {0.f}, {1.f}}; //y=0;
                A::Line bottom = A::Line{A::Scalar{-1080.f}, {0.f}, {1.f}}; //y=1080;
                A::Line col = A::Line {-i,{1.f},{0.f}};
                A::Point t = A::wedge(col, top);
                A::Point b = A::wedge(col, bottom);
                res.push_back(A::xcbSegment(t,b));
            }
            for(A::Scalar j = A::Scalar{0.f}; j < A::Scalar{1080.f}; j+=(A::Scalar{1.f/16.f}*A::Scalar{1080.f})){
                A::Line left = A::Line{A::Scalar{0.f}, {1.f}, {0.f}}; //x=0;
                A::Line right = A::Line{A::Scalar{-1920.f}, {1.f}, {0.f}};//x=1920;
                A::Line row = A::Line {-j,{0.f},{1.f}};
                A::Point l = A::wedge(row, left);
                A::Point r = A::wedge(row, right);
                res.push_back(A::xcbSegment(l,r));
            }
            return res; 
        }
        void draw(Surface& surface, xcb_pixmap_t& back_buffer, uint32_t color, std::array<float, 2> origin, std::array<std::array<float, 2>,2> abSpan, float N, float M){
            uint32_t    mask = XCB_GC_FOREGROUND;
            auto points = GridPoints(N, M);
            xcb_change_gc(surface.info.connection, surface.xcbContext, mask, &color);
            xcb_poly_segment(surface.info.connection, back_buffer, surface.xcbContext, points.size(), &points[0]);
        }
    };
    struct Matrix{
        std::array<B::Plane, 4> dat;
        const B::Plane operator*(const B::Plane a)const{
            return B::Plane{
                B::Scalar{ 
                    (dat[0].dat[0]*a.dat[0])+
                        (dat[1].dat[0]*a.dat[1])+
                        (dat[2].dat[0]*a.dat[2])+
                        (dat[3].dat[0]*a.dat[3])
                },
                    B::Scalar{
                        (dat[0].dat[1]*a.dat[0])+
                            (dat[1].dat[1]*a.dat[1])+
                            (dat[2].dat[1]*a.dat[2])+
                            (dat[3].dat[1]*a.dat[3])
                    },
                    B::Scalar{
                        (dat[0].dat[2]*a.dat[0])+
                            (dat[1].dat[2]*a.dat[1])+
                            (dat[2].dat[2]*a.dat[2])+
                            (dat[3].dat[2]*a.dat[3])},
                    B::Scalar{
                        (dat[0].dat[3]*a.dat[0])+
                            (dat[1].dat[3]*a.dat[1])+
                            (dat[2].dat[3]*a.dat[2])+
                            (dat[3].dat[3]*a.dat[3])
                    },
            };
        }
    };
    struct ControllerInput{
        std::array<float, 2> maxXY;
        std::array<float, 2> lJoy;
        std::array<float, 2> rJoy;
        int a;
        void read (libevdev *dev){
            struct input_event ev;
            int rc=0;
            while((rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev)) != -EAGAIN){
                std::string evType = libevdev_event_type_get_name(ev.type);
                std::string evCode = libevdev_event_code_get_name(ev.type, ev.code);
                if (rc == 0) {
                    //printf("Event: %s %s %d\n",
                    //        evType,
                    //        evCode,
                    //        ev.value);
                }
                if (evType=="EV_ABS"){
                    std::cout << "abs event: "<< evCode << ": " << ev.value <<"\n";
                    if(evCode=="ABS_X"){
                        maxXY[0] = std::max((double)maxXY[0], std::abs((double)ev.value));
                        lJoy[0] = ev.value/maxXY[0];
                    }
                    if(evCode=="ABS_Y"){
                        maxXY[1] = std::max((double)maxXY[1], std::abs((double)ev.value));
                        lJoy[1] = ev.value/maxXY[1];
                    }
                    if(evCode=="ABS_RX"){
                        maxXY[0] = std::max((double)maxXY[0], std::abs((double)ev.value));
                        rJoy[0] = ev.value/maxXY[0];
                    }
                    if(evCode=="ABS_RY"){
                        maxXY[1] = std::max((double)maxXY[1], std::abs((double)ev.value));
                        rJoy[1] = ev.value/maxXY[1];
                    }
                }
                if (evType=="EV_KEY"){
                    std::cout << "key event: "<< evCode << ": " << ev.value <<"\n";
                }
            };
        };
    };
    struct Model{
        std::vector<B::Point> TPose;
        std::string path;
        std::array<float, 3> position;
        int updateCount;
        bool hand;
        float phi;
        std::array<xcb_segment_t, 3> Segment(std::array<B::Point, 3>& tri){
            return {
                B::xcbSegment(tri[0], tri[1]),
                    B::xcbSegment(tri[2], tri[0]),
                    B::xcbSegment(tri[1], tri[2]),
            };
        }
        std::vector<B::Point> loadModel(const std::string& MODEL_PATH) {
            std::vector<B::Point> vertices;
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;
            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
                throw std::runtime_error(warn + err);
            }
            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    vertices.push_back({
                            attrib.vertices[3 * index.vertex_index + 2],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 0],
                            1.f
                            });
                    auto texCoord = std::array<float, 2>{
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };
                    auto color = std::array<float, 3>{1.0f, 1.0f, 1.0f};
                }
            }
            path = MODEL_PATH; 
            TPose = vertices;
            return vertices;
        }
        xcb_segment_t xcbSegment(const B::Point a, const B::Point b){
            return xcb_segment_t{
                .x1 = static_cast<int16_t>( 1920.f*((a.dat[2]/a.dat[3]).val+1.f)/2.f ),
                    .y1 = static_cast<int16_t>(1080.f*((a.dat[1]/a.dat[3]).val+1.f)/2.f ),
                    .x2 = static_cast<int16_t>(1920.f*((b.dat[2]/b.dat[3]).val+1.f)/2.f ),
                    .y2 = static_cast<int16_t>( 1080.f*((b.dat[1]/b.dat[3]).val+1.f)/2.f ),
            };
        }
        std::vector<xcb_segment_t> GridPoints(B::Point camp, float pitch){
            std::array<float, 3> xyz = camp.xyz();
            float theta=0.f;
            std::array<B::Point, 4> v{
                B::Point { -1.f, -0.f, -1.f, 1.f},
                    B::Point { -1.f, -0.f,  1.f, 1.f},
                    B::Point {  1.f, -0.f, -1.f, 1.f},
                    B::Point {  1.f, -0.f,  1.f, 1.f},
            };
            std::array<B::Plane, 8> vd{};
            auto dual=[](const B::Point p){return B::Plane{p.dat[2], p.dat[1], p.dat[0], p.dat[3]}; };
            auto ddual=[](const B::Plane p){return B::Point{p.dat[2], p.dat[1], p.dat[0], p.dat[3]}; };
            std::transform(v.begin(), v.end(), vd.begin(), dual);
            auto T = Matrix{ .dat = {
                B::Plane { B::Scalar{ 1.f}, { 0.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 1.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 0.f}, { 1.f}, {0.f} },
                B::Plane { B::Scalar{ -xyz[0]}, { -xyz[1]}, { -xyz[2]}, {1.f} },
            }
            };
            auto S = Matrix{ .dat = {
                B::Plane { B::Scalar{ 1.f}, { 0.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 1.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 0.f}, { 1.f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 0.f}, { 0.f}, {1.f} },
            }
            };
            auto Ry = Matrix{ .dat = {
                B::Plane { B::Scalar{ (float)cos(theta)}, { 0.f}, {(float)-sin(theta)}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 1.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ (float)sin(theta)}, { 0.f}, { (float)cos(theta)}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 0.f}, {0.f}, {1.f} }}
            };
            auto Rx = Matrix{ .dat = {
                B::Plane { B::Scalar{1.f}, { 0.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{0.f}, {(float)cos(pitch)}, {(float)-sin(pitch)}, {0.f} },
                B::Plane { B::Scalar{0.f}, {(float)sin(pitch)}, { (float)cos(pitch)}, {0.f} },
                B::Plane { B::Scalar{0.f}, { 0.f}, {0.f}, {1.f} } }
            };
            auto Perspective = [](float zn, float zf, float theta, float aspectRatio){
                float r = zn*tan(theta);
                float l = -r;
                float t = r/aspectRatio;
                float b = -t;
                return Matrix{ .dat = {
                    B::Plane { 2.f*zn/(r-l), 0.f, 0.f, 0.f },
                    B::Plane { 0.f, 2.f*zn/(b-t), { 0.f}, {0.f} },
                    B::Plane { (r+l)/(r-l), (b+t)/(b-t), zf/(zf-zn), {1.f} },
                    B::Plane { 0.f, { 0.f}, { -(zf*zn)/(zf-zn)}, {0.f} },
                }};
            };
            auto P = Perspective(1.f, 8.f, 3.14159f/4.f, 1920.f/1080.f);
            std::transform(vd.begin(), vd.end(), vd.begin(), [&S, &Ry, &Rx, &P, &T](const B::Plane& p){ return P*(Rx*(T*(S*(Ry*p)))); });
            std::transform(vd.begin(), vd.end(), v.begin(), ddual);
            auto res = std::vector<xcb_segment_t>{
                xcbSegment(v[0],v[1]),
                    xcbSegment(v[2],v[3]),
                    xcbSegment(v[0],v[2]),
                    xcbSegment(v[1],v[3]),
            };
            return res;
        }
        std::vector<xcb_segment_t> Points(B::Point cam, float pitch){
            auto xyz = cam.xyz();
            float theta=0.f;
            //std::array<Scalar, 4> dat; //e021,e013,e032,e123
            std::vector<B::Point> vertices = TPose;
            std::vector<B::Point> v{};
            std::copy(vertices.begin(), vertices.end(), std::back_inserter(v));
            std::vector<B::Plane> vd{};
            auto dual=[](const B::Point p){return B::Plane{p.dat[2], p.dat[1], p.dat[0], p.dat[3]}; };
            auto ddual=[](const B::Plane p){return B::Point{p.dat[2], p.dat[1], p.dat[0], p.dat[3]}; };
            std::transform(v.begin(), v.end(), std::back_inserter(vd), dual);
            auto T = Matrix{ .dat = {
                B::Plane { B::Scalar{ 1.f}, { 0.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 1.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 0.f}, { 1.f}, {0.f} },
                B::Plane { B::Scalar{ position[0]-xyz[0]}, { position[1]-xyz[1]}, { position[2]-xyz[2]}, {1.f} },
            }
            };
            auto S = Matrix{ .dat = {
                B::Plane { B::Scalar{ hand?0.1f:-0.1f}, { 0.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { -0.1f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 0.f}, { 0.1f}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 0.f}, { 0.f}, {1.f} },
            }
            };
            auto Rx = Matrix{ .dat = {
                B::Plane { B::Scalar{1.f}, { 0.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{0.f}, {(float)cos(pitch)}, {(float)-sin(pitch)}, {0.f} },
                B::Plane { B::Scalar{0.f}, {(float)sin(pitch)}, { (float)cos(pitch)}, {0.f} },
                B::Plane { B::Scalar{0.f}, { 0.f}, {0.f}, {1.f} }
            }
            };
            auto Ry = [](float phi){ return Matrix{ .dat = {
                B::Plane { B::Scalar{ (float)cos(phi)}, { 0.f}, {(float)-sin(phi)}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 1.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ (float)sin(phi)}, { 0.f}, { (float)cos(phi)}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 0.f}, {0.f}, {1.f} }}
            }; };
            auto Rz = Matrix{ .dat = {
                B::Plane { B::Scalar{ (float)cos(theta)}, { 0.f}, {(float)-sin(theta)}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 1.f}, { 0.f}, {0.f} },
                B::Plane { B::Scalar{ (float)sin(theta)}, { 0.f}, { (float)cos(theta)}, {0.f} },
                B::Plane { B::Scalar{ 0.f}, { 0.f}, {0.f}, {1.f} }}
            };
            auto Perspective = [](float zn, float zf, float theta, float aspectRatio){
                float r = zn*tan(theta);
                float l = -r;
                float t = r/aspectRatio;
                float b = -t;
                return Matrix{ .dat = {
                    B::Plane { 2.f*zn/(r-l), 0.f, 0.f, 0.f },
                    B::Plane { 0.f, 2.f*zn/(b-t), { 0.f}, {0.f} },
                    B::Plane { (r+l)/(r-l), (b+t)/(b-t), zf/(zf-zn), {1.f} },
                    B::Plane { 0.f, { 0.f}, { -(zf*zn)/(zf-zn)}, {0.f} },
                }};
            };
            auto P = Perspective(1.f, 8.f, 3.14159f/4.f, 1920.f/1080.f);
            std::transform(vd.begin(), vd.end(), vd.begin(), [&S, &Ry, &Rx, &P, &T, this](const B::Plane& p){ return P*(Rx*(T*(Ry(phi)*(S*p)))); });
            std::transform(vd.begin(), vd.end(), v.begin(), ddual);
            auto res = std::vector<xcb_segment_t>{};
            for(size_t i = 0; i < v.size(); i+=3){
                res.push_back(xcbSegment(v[i+0], v[i+1]));
                res.push_back(xcbSegment(v[i+2], v[i+0]));
                res.push_back(xcbSegment(v[i+1], v[i+2]));
            }
            return res;
        }
        void update(ControllerInput& controller){
            updateCount++;
            hand = ((updateCount%60)==0)?!hand:hand;
            auto subjectTranslator = [](ControllerInput& cont){
                std::array<float, 3> res{};
                if(((cont.lJoy[0]*cont.lJoy[0])+(cont.lJoy[1]*cont.lJoy[1])) > 0.2f){ 
                    res[0]=cont.lJoy[0];
                    res[2]=cont.lJoy[1]; }
                return res;
            };
            auto subjectAngle = [](ControllerInput& controller, float phi){
                if(((controller.lJoy[0]*controller.lJoy[0])+(controller.lJoy[1]*controller.lJoy[1])) > 0.2f){ 
                    phi = (float)atan2(controller.lJoy[1], controller.lJoy[0])-(3.14159f/2.f);
                }
                return phi;
            };
            auto t = subjectTranslator(controller);
            position[0]-=0.04f*t[0];
            position[2]+=0.04f*t[2];

            phi = subjectAngle(controller, phi);
            if(((t[0]*t[0])+(t[2]*t[2]))>0.1f){
                if(path!="figureWalking.obj"){
                    loadModel("figureWalking.obj");
                }
            }
            if(((t[0]*t[0])+(t[2]*t[2]))<0.1f){
                if(path!="figureStanding.obj"){
                    loadModel("figureStanding.obj");
                }
            }
        }
    };
    struct Camera{
        float theta;
        float pitch;
        std::array<float, 2> xy;
        B::Point xyz;
        B::Plane up;
        B::Plane right;
        void update(ControllerInput& controller){
            if(((controller.rJoy[0]*controller.rJoy[0])+(controller.rJoy[1]*controller.rJoy[1])) > 0.2f){ 
                xyz.dat[0]-=0.1f*controller.rJoy[0];
                xyz.dat[2]+=0.1f*controller.rJoy[1];
            }
        }
    };
    Instance instance;
    std::vector<Physical> physical;
    Logical logical;
    Swapchain swapchain;
    void init(bool debug=false){
        instance = createInstance(debug?validationLayers:noLayers);
        auto windowDimension = std::array<uint16_t, 2>{800, 600};
        createSurface(windowDimension, "Vulkan Engine");
        PhysicalDeviceList();
        logical = createLogicalDevice(physical[0], validationLayers, deviceExtensions);
        swapchain = createSwapchain(physical[0], surface, windowDimension);
    };
};
void draw(Application::Surface& surface, uint32_t color, Application::Camera& camera, Application::Model& model){
    uint32_t    mask = XCB_GC_FOREGROUND;
    auto points = model.GridPoints(camera.xyz, camera.pitch);
    auto gpoints = model.Points(camera.xyz, camera.pitch);
    xcb_change_gc(surface.info.connection, surface.xcbContext, mask, &color);
    xcb_poly_segment(surface.info.connection, surface.xcbBuffer, surface.xcbContext, points.size(), &points[0]);
    xcb_poly_segment(surface.info.connection, surface.xcbBuffer, surface.xcbContext, gpoints.size(), &gpoints[0]);
}
int main(int argc, char *argv[]) {
    auto instance = Application{};
    instance.init();
    struct libevdev *dev = NULL;
    auto displayText = [&dev, &instance](int argc, char *argv[]){
        int fd = open("/dev/input/event7", O_RDONLY | O_NONBLOCK);  // Replace "X" with your device number
        int rc = libevdev_new_from_fd(fd, &dev);
        if (rc < 0) {
            fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
            _exit(1);
        }
        printf("Input device name: \"%s\"\n", libevdev_get_name(dev));

        int server_fd = socket(AF_INET, SOCK_STREAM, 0);

        sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(12345);

        bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
        listen(server_fd, 1);

        std::cout << "Waiting for connection..." << std::endl;
        int client_fd = accept(server_fd, nullptr, nullptr);
        std::cout << "Connected!" << std::endl;

        char buffer[1024];

        std::array<Application::Input, 2> in{};
        Application::Model model{};
        model.TPose=model.loadModel("figure.obj");
        model.position = std::array<float, 3>{};
        model.phi = 0.f;
        Application::Model model2{};
        model2.TPose=model.loadModel("figure.obj");
        model2.position = std::array<float, 3>{0.f, 0.f, -4.f};
        model2.phi = 0.f;
        Application::Model model3{};
        model3.TPose=model.loadModel("house.obj");
        model3.position = std::array<float, 3>{0.f, 0.f, 0.f};
        model3.phi = 0.f;
        auto camera = Application::Camera{ .pitch = 0.3f, .xyz=B::Point{0.f, -1.5f, 2.f, 1.f} };
        Application::ControllerInput controller{};
        uint32_t color = 0xFFFF0000;
        for(;;) {
            draw(instance.surface, 0xFF000000, camera, model);
            draw(instance.surface, 0xFF000000, camera, model2);
            draw(instance.surface, 0xFF000000, camera, model3);
            controller.read(dev);
            camera.update(controller);
            model.update(controller);

            bool data = false;
            int bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break;
            }
            // Handle received data here...
            std::cout << "Received data: " << buffer << std::endl;
            std::cout << "nextFrame\n";
            color = (buffer[0]=='l')?0xFF00FF00:0xFFFF0000;
            color = (buffer[0]=='r')?0xFF0000FF:0xFFFF0000;

            draw(instance.surface, 0xFFFFFFFF, camera, model);
            draw(instance.surface, color, camera, model2);
            draw(instance.surface, 0xFFFFFFFF, camera, model3);
            in = instance.poll(in);
            for(int i = 0; i < argc; i++){
                const char* text = argv[i];
                xcb_image_text_8(instance.surface.info.connection, strlen(text), instance.surface.xcbBuffer, instance.surface.xcbContext, 100, 40+(15*i), text);
            }
            xcb_copy_area(instance.surface.info.connection, instance.surface.xcbBuffer, instance.surface.info.window, instance.surface.xcbContext, 0, 0, 0, 0, 1980, 1080);
            xcb_flush(instance.surface.info.connection);
            usleep(1000);
        }
        xcb_close_font(instance.surface.info.connection, instance.surface.font);
        xcb_free_pixmap(instance.surface.info.connection, instance.surface.xcbBuffer);
        libevdev_free(dev);
        close(fd);
        close(client_fd);
        close(server_fd);
    };
    displayText(argc, argv);

    return 0;
}
