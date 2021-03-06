/**
 @author Contributions from the community; see CONTRIBUTORS.md
 @date 2005-2016
 @copyright MPL2; see LICENSE.txt
*/

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

//! internal undo manager state is one of these constants
typedef NS_ENUM(NSInteger, GCUndoManagerState) {
	kGCUndoCollectingTasks = 0,
	kGCUndoIsUndoing = 1,
	kGCUndoIsRedoing = 2
};

typedef NS_ENUM(NSInteger, GCUndoTaskCoalescingKind) {
	kGCCoalesceLastTask = 0,
	kGCCoalesceAllMatchingTasks = 1
};

@class GCUndoGroup, GCUndoManagerProxy, GCConcreteUndoTask;

// the undo manager is a public-API compatible replacement for NSUndoManager but features a simpler internal implementation, some bug fixes and less
// fragility than NSUndoManager. It can be used with NSDocument's -setUndoManager: method (cast to id or NSUndoManager). However its compatibility with
// Core Data is unknown and untested at this time. See further notes at the end of this file.

/** @brief This class is a public API-compatible replacement for NSUndoManager.

This class is a public API-compatible replacement for NSUndoManager. It can only be used with Appkit however, not with other types of executable.
 
The point of this is to provide an undo manager whose source is openly readable, available and debuggable. It also does not exhibit the
 \c NSUndoManager bug whereby opening and closing a group without adding any tasks creates an empty task. That substantially simplifies how
 it can be used in an interactive situation such as handling the mouse down/drag/up triplet of views.
 
 This also includes task coalescing whereby consecutive tasks having the same target and selector are only submitted to the stack once. This
 helps a lot with interactive tasks involving multiple events such as mouse dragging, so that undo does not replay all the intermediate steps.
 
 Instances of this can be used as well as \c NSUndoManager if required. This handles all of its own event loop observing and automatic open
 and close of groups independently of the standard mechanism.
 
 Otherwise this should behave identically to \c NSUndoManager when used in an application, except as noted below.
 
 The sending of notifications is not quite as it appears to be documented for NSUndoManager. If you implement as documented, the
 change count for \c NSDocument is not managed correctly. Instead, this sends notifications in a manner that appears to be what \c NSUndoManager
 actually does, and so \c NSDocument change counts work as they should. Also, the purpose and exact usage of \c NSCheckPointNotification is
 unclear so while this follows the documentation, any code relying on this vague notification might not work correctly.
 
 \c -undoNestedGroup only operates on top level groups in this implementation, and is thus functionally equivalent to <code>-undo</code>. In fact \c -undo simply
 calls \c -undoNestedGroup here.
*/
@interface GCUndoManager : NSObject {
@private
	NSMutableArray<GCUndoGroup*>* mUndoStack; // list of groups making up the undo stack
	NSMutableArray<GCUndoGroup*>* mRedoStack; // list of groups making up the redo stack
	NSArray* mRunLoopModes; // current run loop modes, used by automatic grouping by event
	id mNextTarget; // next prepared target
	GCUndoGroup* mOpenGroupRef; // internal reference to current open group
	GCUndoManagerProxy* mProxy; //!< the proxy object returned by \c -prepareWithInvocationTarget: if proxying is used
	NSInteger mGroupLevel; // current grouping level, 0 = no groups open
	NSUInteger mLevelsOfUndo; // how many undo actions are added before old ones are discarded, 0 = unlimited
	NSInteger mEnableLevel; // enable ref count, 0 = enabled.
	NSUInteger mChangeCount; // count of changes (submitting any task increments this)
	GCUndoManagerState mState; // current undo manager state
	GCUndoTaskCoalescingKind mCoalKind; // coalescing behaviour - match on emost recent task or all tasks in group
	BOOL mGroupsByEvent; // YES if automatic grouping occurs for the main loop event cycle
	BOOL mCoalescing; // YES if consecutive tasks are coalesced
	BOOL mAutoDeleteEmptyGroups; // YES if empty groups are automatically removed from the stack
	BOOL mRetainsTargets; // YES if invocation targets are retained
	BOOL mIsRemovingTargets; // YES during stack clean-up to prevent re-entrancy
}

- (instancetype)init;

// NSUndoManager compatible API
// undo groups

- (void)beginUndoGrouping;
- (void)endUndoGrouping;

@property (readonly) NSInteger groupingLevel;
@property BOOL groupsByEvent;

/** n.b. if this is changed while a callback is pending, the new modes won't take effect until
 the next event cycle.
 */
@property (copy) NSArray<NSRunLoopMode>* runLoopModes;

// enabling undo registration

- (void)enableUndoRegistration;
- (void)disableUndoRegistration;
@property (readonly, getter=isUndoRegistrationEnabled) BOOL undoRegistrationEnabled;

/** @brief setting the number of undos allowed before old ones are discarded
 */
@property (nonatomic) NSUInteger levelsOfUndo;

// performing the undo or redo

@property (readonly) BOOL canUndo;
@property (readonly) BOOL canRedo;

- (void)undo;
- (void)redo;
- (void)undoNestedGroup;

@property (readonly, getter=isUndoing) BOOL undoing;
@property (readonly, getter=isRedoing) BOOL redoing;

// undo menu management

- (void)setActionName:(NSString*)actionName;
@property (readonly, copy) NSString* undoActionName;
@property (readonly, copy) NSString* redoActionName;
@property (readonly, copy) NSString* undoMenuItemTitle;
@property (readonly, copy) NSString* redoMenuItemTitle;
- (NSString*)undoMenuTitleForUndoActionName:(NSString*)actionName;
- (NSString*)redoMenuTitleForUndoActionName:(NSString*)actionName;

// registering actions with the undo manager

- (id)prepareWithInvocationTarget:(id)target;
- (void)forwardInvocation:(NSInvocation*)invocation;
- (void)registerUndoWithTarget:(id)target selector:(SEL)selector object:(nullable id)anObject;

// removing actions

- (void)removeAllActions;
- (void)removeAllActionsWithTarget:(id)target;

// private NSUndoManager API for compatibility

- (void)_processEndOfEventNotification:(NSNotification*)note;

// additional API

// automatic empty group discarding (default = YES)
/** @brief automatic empty group discarding (default = YES)
 
 set whether empty groups are automatically discarded when the top level group is closed. Default is YES. Set to
 \c NO for NSUndoManager behaviour - could conceivably be used to trigger undo managed outside of the undo manager.
 However this behaviour is buggy for normal usage of the undo manager. Setting this from \c NO to \c YES does not
 remove existing empty groups. Used in -endUndoGrouping.
 */
@property BOOL automaticallyDiscardsEmptyGroups;

// task coalescing (default = NO)

- (void)enableUndoTaskCoalescing;
- (void)disableUndoTaskCoalescing;
@property (readonly, getter=isUndoTaskCoalescingEnabled) BOOL undoTaskCoalescingEnabled;

/**
 
 The behaviour for coalescing. \c kGCCoalesceLastTask (default) checks just the most recent task submitted, whereas
 \c kGCCoalesceAllMatchingTasks checks all in the current group. Last task is appropriate for property changes such as
 ABBBBBBA > ABA, where the last A needs to be included but the intermediate B's do not. The other kind is better for changes
 such as ABABABAB > AB where a repeated sequence is coalesced into a single example of the sequence.
*/
@property GCUndoTaskCoalescingKind coalescingKind;

// retaining targets

@property BOOL retainsTargets;
- (void)setNextTarget:(id)target;

// getting/resetting change count

/**
 return the change count, which is roughly the number of individual tasks accepted. However, do not rely on the exact value,
 instead you can compare it before and after, and if it has changed, then something was added. This could be used to e.g.
 provide some additional auxiliary undoable state, such as selection changes, which are not normally considered undoable
 in their own right.
 */
@property (readonly) NSUInteger changeCount;
- (void)resetChangeCount;

// internal methods - public to permit overriding

/** @brief return the currently open group, or \c nil if no group is open
 */
@property (readonly, assign) GCUndoGroup* currentGroup;

@property (readonly, retain) NSArray<GCUndoGroup*>* undoStack;
@property (readonly, retain) NSArray<GCUndoGroup*>* redoStack;

- (nullable GCUndoGroup*)peekUndo;
- (nullable GCUndoGroup*)peekRedo;
@property (readonly) NSUInteger numberOfUndoActions;
@property (readonly) NSUInteger numberOfRedoActions;

- (void)pushGroupOntoUndoStack:(GCUndoGroup*)aGroup;
- (void)pushGroupOntoRedoStack:(GCUndoGroup*)aGroup;

- (BOOL)submitUndoTask:(GCConcreteUndoTask*)aTask;

- (void)popUndoAndPerformTasks;
- (void)popRedoAndPerformTasks;
- (nullable GCUndoGroup*)popUndo;
- (nullable GCUndoGroup*)popRedo;

- (void)clearRedoStack;
- (void)checkpoint;

/** @abstract sets the current state of the undo manager - set internally, not for client use
 */
@property GCUndoManagerState undoManagerState;
- (void)reset;

- (void)conditionallyBeginUndoGrouping;

// debugging utility:

- (void)explodeTopUndoAction;

@end

#pragma mark -

/**
 undo tasks (actions) come in two types - groups and concrete tasks. Both descend from the same semi-abstract base which
 provides the 'back pointer' to the parent group. The \c -perform method must be overridden by concrete subclasses.
 */
@interface GCUndoTask : NSObject {
@private
	GCUndoGroup* mGroupRef;
}

@property (assign) GCUndoGroup* parentGroup;
- (void)perform;

@end

#pragma mark -

/**
 Undo groups can contain any number of other groups or concrete tasks. The top level actions in the undo/redo stacks always consist
 of groups, even if they only contain a single concrete task. The group also provides the storage for the action name associated with
 the action. Groups own their tasks.
 */
@interface GCUndoGroup : GCUndoTask {
@private
	NSString* mActionName;
	NSMutableArray* mTasks;
}

- (void)addTask:(GCUndoTask*)aTask;
- (GCUndoTask*)taskAtIndex:(NSUInteger)indx;
@property (readonly, nullable) GCConcreteUndoTask* lastTaskIfConcrete;
@property (readonly, retain) NSArray<GCUndoTask*>* tasks;
- (NSArray<GCUndoTask*>*)tasksWithTarget:(nullable id)target selector:(nullable SEL)selector;
/** return whether the group contains any actual tasks. If it only contains other empty groups, returns YES.
 */
@property (readonly, getter=isEmpty) BOOL empty;

- (void)removeTasksWithTarget:(id)aTarget undoManager:(GCUndoManager*)um;
/** @brief The group's action name.
 
 In general, setting this is automatically handled by the owning undo manager.
 */
@property (copy) NSString* actionName;

@end

#pragma mark -

// concrete tasks wrap the NSInvocation which embodies the actual method call that is made when an action is undone or redone.
// Concrete tasks own the invocation, which is set to always retain its target and arguments.

@interface GCConcreteUndoTask : GCUndoTask {
@private
	NSInvocation* mInvocation;
	id mTarget;
	BOOL mTargetRetained;
}

- (instancetype)init UNAVAILABLE_ATTRIBUTE;
- (instancetype)initWithInvocation:(NSInvocation*)inv NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithTarget:(id)target selector:(SEL)selector object:(id)object;
- (void)setTarget:(id)target retained:(BOOL)retainIt;
@property (readonly, unsafe_unretained) id target;
@property (readonly) SEL selector;

@end

// macros to throw exceptions (similar to NSAssert but always compiled in)

#ifndef THROW_IF_FALSE
#define THROW_IF_FALSE(condition, string)                                     \
	if (!(condition)) {                                                       \
		[NSException raise:NSInternalInconsistencyException format:(string)]; \
	}
#define THROW_IF_FALSE1(condition, string, param1)                                      \
	if (!(condition)) {                                                                 \
		[NSException raise:NSInternalInconsistencyException format:(string), (param1)]; \
	}
#define THROW_IF_FALSE2(condition, string, param1, param2)                                        \
	if (!(condition)) {                                                                           \
		[NSException raise:NSInternalInconsistencyException format:(string), (param1), (param2)]; \
	}
#endif

NS_ASSUME_NONNULL_END
