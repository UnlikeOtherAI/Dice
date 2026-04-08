#import <Foundation/Foundation.h>
#import <QuartzCore/CAMetalLayer.h>
#import <UIKit/UIKit.h>
#include "dice3d/dice3d.h"

NS_ASSUME_NONNULL_BEGIN

@interface DiceRenderer : NSObject

- (instancetype)init;
- (void)attachLayer:(CAMetalLayer*)layer width:(uint32_t)w height:(uint32_t)h;
- (void)detachLayer;
- (void)resize:(uint32_t)w height:(uint32_t)h;

/// NOTE: Synchronous mesh build — do not call on the main thread for d32.
- (uint32_t)addDieWithSides:(int)sides
                      bevel:(float)bevel
                   dieColor:(UIColor*)color
               whiteNumbers:(BOOL)white;
- (void)removeDie:(uint32_t)handle;
- (void)rollDie:(uint32_t)handle result:(int)result duration:(float)duration;
- (void)tick:(float)dt;
- (void)renderFrame;

@end

NS_ASSUME_NONNULL_END
