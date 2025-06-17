#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface U3MetalRenderer : NSObject

- (instancetype)initWithView:(MTKView *)view;
- (void)draw;

@end

NS_ASSUME_NONNULL_END
