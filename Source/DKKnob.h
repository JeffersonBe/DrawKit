/**
 @author Contributions from the community; see CONTRIBUTORS.md
 @date 2005-2016
 @copyright MPL2; see LICENSE.txt
*/

#import <Cocoa/Cocoa.h>
#import "DKCommonTypes.h"

NS_ASSUME_NONNULL_BEGIN

//! visual flags, used internally
typedef NS_OPTIONS(NSUInteger, DKKnobDrawingFlags) {
	kDKKnobDrawsStroke = (1 << 0),
	kDKKnobDrawsFill = (1 << 1)
};

@class DKHandle;

/** @brief simple class used to provide the drawing of knobs for object selection.

simple class used to provide the drawing of knobs for object selection. You can override this and replace it (attached to any layer)
to customise the appearance of the selection knobs for all drawn objects in that layer.

The main method a drawable will call is drawKnobAtPoint:ofType:userInfo:

The type (DKKnobType) is a functional description of the knob only - this class maps that functional description to a consistent appearance taking
into account the basic type and a couple of generic state flags. Clients should generally avoid trying to do drawing themselves of knobs, but if they do,
should use the lower level methods here to get consistent results.

Subclasses may want to customise many aspects of a knob's appearance, and can override any suitable factored methods according to their needs. Customisations
might include the shape of a knob, its colours, whether stroked or filled or both, etc.
*/
@interface DKKnob : NSObject <NSCoding, NSCopying> {
@private
	__weak id<DKKnobOwner> m_ownerRef; // the object that owns (and hence retains) this - typically a DKLayer
	NSSize m_knobSize; // the currently cached knob size
	CGFloat mScaleRatio; // ratio to zoom factor used to scale knob size (default = 0.3)
	NSColor* mControlKnobColour; // colour of square knobs
	NSColor* mRotationKnobColour; // colour of rotation knobs
	NSColor* mControlOnPathPointColour; // colour of on-path control points
	NSColor* mControlOffPathPointColour; // colour of off-path control points
	NSColor* mControlBarColour; // colour of control bars
	NSSize mControlKnobSize; // control knob size
	CGFloat mControlBarWidth; // control bar width
}

/**  */
+ (instancetype)standardKnobs;

// main high-level methods that will be called by clients

/** the object that owns (and hence retains) this - typically a \c DKLayer
 */
@property (nonatomic, weak) id<DKKnobOwner> owner;

- (void)drawKnobAtPoint:(NSPoint)p ofType:(DKKnobType)knobType userInfo:(nullable id)userInfo;
- (void)drawKnobAtPoint:(NSPoint)p ofType:(DKKnobType)knobType angle:(CGFloat)radians userInfo:(nullable id)userInfo;
- (void)drawKnobAtPoint:(NSPoint)p ofType:(DKKnobType)knobType angle:(CGFloat)radians highlightColour:(nullable NSColor*)aColour;

- (void)drawControlBarFromPoint:(NSPoint)a toPoint:(NSPoint)b;
- (void)drawControlBarWithKnobsFromPoint:(NSPoint)a toPoint:(NSPoint)b;
- (void)drawControlBarWithKnobsFromPoint:(NSPoint)a ofType:(DKKnobType)typeA toPoint:(NSPoint)b ofType:(DKKnobType)typeB;
- (void)drawRotationBarWithKnobsFromCentre:(NSPoint)centre toPoint:(NSPoint)p;
- (void)drawPartcode:(NSInteger)code atPoint:(NSPoint)p fontSize:(CGFloat)fontSize;

- (BOOL)hitTestPoint:(NSPoint)p inKnobAtPoint:(NSPoint)kp ofType:(DKKnobType)knobType userInfo:(nullable id)userInfo;

/** colour of control bars
 */
@property (copy) NSColor* controlBarColour;
/** control bar width
 */
@property CGFloat controlBarWidth;

@property CGFloat scalingRatio;

// low-level methods (mostly internal and overridable)

/** the currently cached knob size
 */
@property NSSize controlKnobSize;
- (void)setControlKnobSizeForViewScale:(CGFloat)scale;

// new model APIs

- (DKHandle*)handleForType:(DKKnobType)knobType;
- (DKHandle*)handleForType:(DKKnobType)knobType colour:(nullable NSColor*)colour;
@property (readonly) NSSize actualHandleSize;

@end

#pragma mark -

@interface DKKnob (Deprecated)

+ (void)setControlKnobColour:(null_unspecified NSColor*)clr;
+ (null_unspecified NSColor*)controlKnobColour;

+ (void)setRotationKnobColour:(null_unspecified NSColor*)clr;
+ (null_unspecified NSColor*)rotationKnobColour;

+ (void)setControlOnPathPointColour:(null_unspecified NSColor*)clr;
+ (null_unspecified NSColor*)controlOnPathPointColour;
+ (void)setControlOffPathPointColour:(null_unspecified NSColor*)clr;
+ (null_unspecified NSColor*)controlOffPathPointColour;

+ (void)setControlBarColour:(null_unspecified NSColor*)clr;
+ (null_unspecified NSColor*)controlBarColour;

+ (void)setControlKnobSize:(NSSize)size;
+ (NSSize)controlKnobSize;

+ (void)setControlBarWidth:(CGFloat)width;
+ (CGFloat)controlBarWidth;

+ (NSRect)controlKnobRectAtPoint:(NSPoint)kp;

- (null_unspecified NSColor*)fillColourForKnobType:(DKKnobType)knobType;
- (null_unspecified NSColor*)strokeColourForKnobType:(DKKnobType)knobType;
- (CGFloat)strokeWidthForKnobType:(DKKnobType)knobType;

// setting colours and sizes per-DKKnob instance

- (void)setControlKnobColour:(null_unspecified NSColor*)clr;
- (null_unspecified NSColor*)controlKnobColour;
- (void)setRotationKnobColour:(null_unspecified NSColor*)clr;
- (null_unspecified NSColor*)rotationKnobColour;

- (void)setControlOnPathPointColour:(null_unspecified NSColor*)clr;
- (null_unspecified NSColor*)controlOnPathPointColour;
- (void)setControlOffPathPointColour:(null_unspecified NSColor*)clr;
- (null_unspecified NSColor*)controlOffPathPointColour;

- (NSRect)controlKnobRectAtPoint:(NSPoint)kp;
- (NSRect)controlKnobRectAtPoint:(NSPoint)kp ofType:(DKKnobType)knobType;

- (null_unspecified NSBezierPath*)knobPathAtPoint:(NSPoint)p ofType:(DKKnobType)knobType angle:(CGFloat)radians userInfo:(null_unspecified id)userInfo;
- (void)drawKnobPath:(null_unspecified NSBezierPath*)path ofType:(DKKnobType)knobType userInfo:(null_unspecified id)userInfo;
- (DKKnobDrawingFlags)drawingFlagsForKnobType:(DKKnobType)knobType;

@end

// keys in the userInfo that can be used to pass additional information to the knob drawing methods

extern NSString* const kDKKnobPreferredHighlightColour; // references an NSColor

NS_ASSUME_NONNULL_END
