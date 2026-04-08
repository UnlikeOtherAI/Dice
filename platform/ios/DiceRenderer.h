#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UIKit/UIKit.h>
#include "dice3d/dice3d.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, DicePresentationMode) {
    DicePresentationModeStatic = 0,
    DicePresentationModeSpinIn = 1,
    DicePresentationModeIdleSpin = 2,
};

@interface DiceRenderer : NSObject

- (instancetype)init;
- (void)attachLayer:(CAMetalLayer*)layer width:(uint32_t)w height:(uint32_t)h;
- (void)detachLayer;
- (void)resize:(uint32_t)w height:(uint32_t)h;
- (void)setCameraDistance:(float)distance;

/// NOTE: Synchronous mesh build — do not call on the main thread for d32.
- (uint32_t)addDieWithSides:(int)sides
                      bevel:(float)bevel
                   dieColor:(UIColor*)color
               whiteNumbers:(BOOL)white;
- (void)removeDie:(uint32_t)handle;
- (void)rollDie:(uint32_t)handle result:(int)result duration:(float)duration;
- (void)setPresentationMode:(DicePresentationMode)mode
                     forDie:(uint32_t)handle
                      speed:(float)speed
                   duration:(float)duration;
- (void)setIdleSpinSpeed:(float)speed forDie:(uint32_t)handle;
- (void)beginDragForDie:(uint32_t)handle;
- (void)dragDie:(uint32_t)handle deltaX:(float)deltaX deltaY:(float)deltaY;
- (void)endDragForDie:(uint32_t)handle;
- (void)tick:(float)dt;
- (void)renderFrame;
- (void)loadAtlasForSides:(int)sides;

@end

NS_ASSUME_NONNULL_END
