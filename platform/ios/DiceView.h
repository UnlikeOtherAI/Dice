#import <UIKit/UIKit.h>
#import "DiceRenderer.h"

NS_ASSUME_NONNULL_BEGIN

/// A UIView subclass that hosts a Filament Metal rendering surface.
/// Add to your view hierarchy, call startRenderLoop, then use .renderer to manage dice.
@interface DiceView : UIView

@property (nonatomic, readonly) DiceRenderer* renderer;

- (void)startRenderLoop;
- (void)stopRenderLoop;

@end

NS_ASSUME_NONNULL_END
