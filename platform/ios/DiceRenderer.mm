#import "DiceRenderer.h"

@implementation DiceRenderer {
    Dice3DSceneRef _scene;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _scene = dice3d_create();
    }
    return self;
}

- (void)dealloc {
    dice3d_detach_surface(_scene);
    dice3d_destroy(_scene);
}

- (void)attachLayer:(CAMetalLayer*)layer width:(uint32_t)w height:(uint32_t)h {
    // __bridge: no ownership transfer — Filament does not own the CAMetalLayer
    void* nativeWindow = (__bridge void*)layer;
    dice3d_attach_surface(_scene, nativeWindow, w, h);
}

- (void)detachLayer {
    dice3d_detach_surface(_scene);
}

- (void)resize:(uint32_t)w height:(uint32_t)h {
    dice3d_resize(_scene, w, h);
}

- (uint32_t)addDieWithSides:(int)sides
                      bevel:(float)bevel
                   dieColor:(UIColor*)color
               whiteNumbers:(BOOL)white {
    CGFloat r, g, b, a;
    [color getRed:&r green:&g blue:&b alpha:&a];
    return dice3d_add_die(_scene, sides, bevel,
                          (float)r, (float)g, (float)b, (float)a,
                          white ? 1 : 0);
}

- (void)removeDie:(uint32_t)handle {
    dice3d_remove_die(_scene, handle);
}

- (void)rollDie:(uint32_t)handle result:(int)result duration:(float)duration {
    dice3d_roll(_scene, handle, result, duration);
}

- (void)tick:(float)dt {
    dice3d_tick(_scene, dt);
}

- (void)renderFrame {
    dice3d_render_frame(_scene);
}

@end
