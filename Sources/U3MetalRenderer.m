#import "U3MetalRenderer.h"

@implementation U3MetalRenderer {
    __weak MTKView *_view;
    id<MTLDevice> _device;
    id<MTLCommandQueue> _commandQueue;
}

- (instancetype)initWithView:(MTKView *)view {
    self = [super init];
    if (self) {
        _view = view;
        _device = MTLCreateSystemDefaultDevice();
        _view.device = _device;
        _commandQueue = [_device newCommandQueue];
    }
    return self;
}

- (void)draw {
    @autoreleasepool {
        id<CAMetalDrawable> drawable = [_view currentDrawable];
        if (!drawable) {
            return;
        }
        id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
}

@end

static U3MetalRenderer *sRenderer = nil;

void U3InitMetalRenderer(MTKView *view) {
    sRenderer = [[U3MetalRenderer alloc] initWithView:view];
}

void U3MetalRenderFrame(void) {
    [sRenderer draw];
}
