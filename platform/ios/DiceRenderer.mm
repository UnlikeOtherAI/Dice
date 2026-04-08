#import "DiceRenderer.h"
#import <CoreGraphics/CoreGraphics.h>

@implementation DiceRenderer {
    Dice3DSceneRef _scene;
    int _loadedAtlasSides;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _scene = dice3d_create();
        _loadedAtlasSides = 0;
        [self _loadBundledMaterial];
        [self loadAtlasForSides:16];
    }
    return self;
}

- (void)_loadBundledMaterial {
    NSURL *url = [[NSBundle mainBundle] URLForResource:@"dice" withExtension:@"filamat"];
    if (!url) return;
    NSData *data = [NSData dataWithContentsOfURL:url];
    if (!data) return;
    dice3d_load_material(_scene, data.bytes, (size_t)data.length);
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

- (void)loadAtlasForSides:(int)sides {
    if (sides == _loadedAtlasSides) return;
    NSString* fileName = [NSString stringWithFormat:@"d%d", sides];
    NSString* path = [[NSBundle mainBundle] pathForResource:fileName ofType:@"png"];
    if (!path) return;

    UIImage *image = [UIImage imageWithContentsOfFile:path];
    CGImageRef cgImage = image.CGImage;
    if (!cgImage) return;

    size_t width = CGImageGetWidth(cgImage);
    size_t height = CGImageGetHeight(cgImage);
    if (width == 0 || height == 0) return;

    size_t bytesPerRow = width * 4;
    size_t byteCount = bytesPerRow * height;
    NSMutableData *pixels = [NSMutableData dataWithLength:byteCount];
    if (pixels.length != byteCount) return;

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        pixels.mutableBytes,
        width,
        height,
        8,
        bytesPerRow,
        colorSpace,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big
    );
    if (!context) {
        CGColorSpaceRelease(colorSpace);
        return;
    }

    CGContextTranslateCTM(context, 0, (CGFloat)height);
    CGContextScaleCTM(context, 1.0f, -1.0f);
    CGContextDrawImage(context, CGRectMake(0, 0, (CGFloat)width, (CGFloat)height), cgImage);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);

    dice3d_load_atlas(_scene, pixels.bytes, (uint32_t)width, (uint32_t)height);
    _loadedAtlasSides = sides;
}

- (void)resize:(uint32_t)w height:(uint32_t)h {
    dice3d_resize(_scene, w, h);
}

- (void)setCameraDistance:(float)distance {
    dice3d_set_camera_distance(_scene, distance);
}

- (uint32_t)addDieWithSides:(int)sides
                      bevel:(float)bevel
                   dieColor:(UIColor*)color
               whiteNumbers:(BOOL)white {
    [self loadAtlasForSides:sides];
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

- (void)setPresentationMode:(DicePresentationMode)mode
                     forDie:(uint32_t)handle
                      speed:(float)speed
                   duration:(float)duration {
    dice3d_set_presentation_mode(
        _scene,
        handle,
        (Dice3DPresentationMode)mode,
        speed,
        duration
    );
}

- (void)setIdleSpinSpeed:(float)speed forDie:(uint32_t)handle {
    dice3d_set_idle_spin_speed(_scene, handle, speed);
}

- (void)setSelectionFlashEnabled:(BOOL)enabled forDie:(uint32_t)handle {
    dice3d_set_selection_flash_enabled(_scene, handle, enabled ? 1 : 0);
}

- (void)beginDragForDie:(uint32_t)handle {
    dice3d_begin_drag(_scene, handle);
}

- (void)dragDie:(uint32_t)handle deltaX:(float)deltaX deltaY:(float)deltaY {
    dice3d_drag_by(_scene, handle, deltaX, deltaY);
}

- (void)endDragForDie:(uint32_t)handle {
    dice3d_end_drag(_scene, handle);
}

- (void)tick:(float)dt {
    dice3d_tick(_scene, dt);
}

- (void)renderFrame {
    dice3d_render_frame(_scene);
}

@end
