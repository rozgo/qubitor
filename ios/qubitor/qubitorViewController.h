#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

#import "qb.h"

typedef enum {
    
    TOOL_PICK,
    MODEL_PICK,
    WORLD_PICK,
    
} PICK_TYPE;

@interface qubitorViewController : UIViewController <UIGestureRecognizerDelegate> {
    
    EAGLContext *context;
    GLuint point_program;
    GLuint solid_program;
    GLuint line_program;
    
    BOOL animating;
    NSInteger animationFrameInterval;
    CADisplayLink *displayLink;
    
    NSDate* epoch;
    
    context_t* world_ctx;
    context_t* model_ctx;
    context_t* gamut_ctx;
    context_t* tools_ctx;
    
    BOOL touchMoved;
    octant_t* touchedGamut;
    octant_t* touchedWorld;
    
    float dt;
}

@property (nonatomic, retain) IBOutlet UILabel* statusBar;
@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

- (void)startAnimation;
- (void)stopAnimation;

@end
