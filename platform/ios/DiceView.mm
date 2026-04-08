#import "DiceView.h"
#import <QuartzCore/CAMetalLayer.h>

@implementation DiceView {
    CADisplayLink* _displayLink;
    CFTimeInterval _lastTime;
    DiceRenderer*  _renderer;
}

@synthesize renderer = _renderer;

+ (Class)layerClass {
    return [CAMetalLayer class];
}

- (void)startRenderLoop {
    _renderer = [[DiceRenderer alloc] init];

    CAMetalLayer* layer = (CAMetalLayer*)self.layer;
    layer.pixelFormat       = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly   = YES;  // default; transparency via opaque=NO + Filament CONFIG_TRANSPARENT
    layer.opaque            = NO;

    CGSize s = self.bounds.size;
    uint32_t w = (uint32_t)(s.width  * self.contentScaleFactor);
    uint32_t h = (uint32_t)(s.height * self.contentScaleFactor);
    layer.drawableSize = CGSizeMake(w, h);
    [_renderer attachLayer:layer width:w height:h];

    _lastTime = CACurrentMediaTime();
    _displayLink = [CADisplayLink displayLinkWithTarget:self
                                              selector:@selector(_tick)];
    _displayLink.preferredFramesPerSecond = 60;
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                      forMode:NSRunLoopCommonModes];
}

- (void)_tick {
    CFTimeInterval now = CACurrentMediaTime();
    float dt = (float)(now - _lastTime);
    _lastTime = now;
    [_renderer tick:dt];
    [_renderer renderFrame];
}

- (void)stopRenderLoop {
    [_displayLink invalidate];
    _displayLink = nil;
    [_renderer detachLayer];
}

- (void)layoutSubviews {
    [super layoutSubviews];
    // Update drawable size on bounds change
    if (_renderer) {
        CGSize s = self.bounds.size;
        CAMetalLayer* metalLayer = (CAMetalLayer*)self.layer;
        UIScreen* screen = self.window ? self.window.screen : UIScreen.mainScreen;
        CGFloat scale = metalLayer.contentsScale;
        if (screen.bounds.size.width > 0.0) {
            scale = screen.nativeBounds.size.width / screen.bounds.size.width;
        }
        if (scale < 1.0) scale = 1.0;
        metalLayer.contentsScale = scale;
        uint32_t w = (uint32_t)(s.width  * scale);
        uint32_t h = (uint32_t)(s.height * scale);
        metalLayer.drawableSize = CGSizeMake(w, h);
        [_renderer resize:w height:h];
    }
}

@end
