#import <UIKit/UIKit.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

//#define MULTI_SAMPLE

@class EAGLContext;

// This class wraps the CAEAGLLayer from CoreAnimation into a convenient UIView subclass.
// The view content is basically an EAGL surface you render your OpenGL scene into.
// Note that setting the view non-opaque will only work if the EAGL surface has an alpha channel.
@interface EAGLView : UIView {
    // The pixel dimensions of the CAEAGLLayer.
    GLint framebufferWidth;
    GLint framebufferHeight;
    
#ifdef MULTI_SAMPLE
    GLuint resolveFramebuffer, resolveColorbuffer, msaaFramebuffer, msaaColorbuffer, msaaDepthbuffer;
#else
    GLuint defaultFramebuffer, colorRenderbuffer, depthbuffer;
#endif
}

@property (nonatomic, retain) EAGLContext *context;
@property GLint framebufferWidth;
@property GLint framebufferHeight;

- (void)setFramebuffer;
- (BOOL)presentFramebuffer;

@end
