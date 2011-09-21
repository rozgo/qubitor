#import <QuartzCore/QuartzCore.h>
#import "qubitorViewController.h"
#import "EAGLView.h"
#import "world.h"
#import "model.h"
#import "gamut.h"
#import "tools.h"

@interface qubitorViewController ()
@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) CADisplayLink *displayLink;
- (BOOL)loadPointShaders;
- (BOOL)loadLineShaders;
- (BOOL)loadSolidShaders;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file;
- (BOOL)linkProgram:(GLuint)prog;
- (BOOL)validateProgram:(GLuint)prog;
+ (qube_t*)qubeFromImage:(NSString*)path;
CGFloat CGPointDist(CGPoint point1,CGPoint point2);
@end

@implementation qubitorViewController

@synthesize animating;
@synthesize context;
@synthesize displayLink;
@synthesize statusBar;

- (void)awakeFromNib
{
    EAGLContext *aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    if (!aContext)
        NSLog(@"Failed to create ES context");
    else if (![EAGLContext setCurrentContext:aContext])
        NSLog(@"Failed to set ES context current");
    
	self.context = aContext;
	[aContext release];
	
    [(EAGLView *)self.view setContext:context];
    [(EAGLView *)self.view setFramebuffer];
    
    [self.view addSubview:self.statusBar];
    self.statusBar.frame = CGRectMake(0, 0, ((EAGLView *)self.view).framebufferWidth, 10);
    
    epoch = [[NSDate date] retain];
    
    qb_qubes_init ( 128 );
    //qb_cuboids_init ( 1024 );
    qb_cuboids_init ( 4096 );
    qb_octants_init ( 65536 );
    qb_render_init ();
    
    world_ctx = ( context_t* )qb_world_context_create ();
    world_ctx->viewport[0] = 0;
    world_ctx->viewport[1] = 0;
    world_ctx->viewport[2] = ((EAGLView *)self.view).framebufferWidth;
    world_ctx->viewport[3] = ((EAGLView *)self.view).framebufferHeight;
    
    model_ctx = ( context_t* )qb_model_context_create ();
    model_ctx->viewport[0] = 0;
    model_ctx->viewport[1] = 0;
    model_ctx->viewport[2] = ((EAGLView *)self.view).framebufferWidth;
    model_ctx->viewport[3] = ((EAGLView *)self.view).framebufferHeight;
    
    gamut_ctx = ( context_t* )qb_gamut_context_create ();
    gamut_ctx->viewport[0] = 0;
    gamut_ctx->viewport[1] = 0;
    gamut_ctx->viewport[2] = ((EAGLView *)self.view).framebufferWidth;
    gamut_ctx->viewport[3] = ((EAGLView *)self.view).framebufferHeight;
    
    tools_ctx = ( context_t* )qb_tools_context_create ();
    tools_ctx->viewport[0] = 0;
    tools_ctx->viewport[1] = 0;
    tools_ctx->viewport[2] = ((EAGLView *)self.view).framebufferWidth;
    tools_ctx->viewport[3] = ((EAGLView *)self.view).framebufferHeight;
    
    tools_context_t* ttx = ( tools_context_t* )tools_ctx;
    ttx->world_ctx = world_ctx;
    ttx->model_ctx = model_ctx;
    ttx->gamut_ctx = gamut_ctx;
    
    qb_tools_setup ( tools_ctx );
    qb_world_setup ( world_ctx );
    qb_model_setup ( model_ctx );
    qb_gamut_setup ( gamut_ctx );
    
    [self loadPointShaders];
    [self loadSolidShaders];
    [self loadLineShaders];
    
    animating = FALSE;
    animationFrameInterval = 1;
    self.displayLink = nil;
}

static float time_elapsed = 0;
vec_t qb_timer_elapsed () {
    return time_elapsed;
}

- (void)dealloc
{
    if ( point_program )
    {
        glDeleteProgram ( point_program );
        point_program = 0;
    }
    
    if ( line_program ) {
        glDeleteProgram ( line_program );
        line_program = 0;
    }
    
    [epoch release];
    
    // Tear down context.
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
    
    [context release];
    
    [super dealloc];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc. that aren't in use.
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return TRUE;
}

- (void)viewWillAppear:(BOOL)animated
{
    [self startAnimation];
    
    [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self stopAnimation];
    
    [super viewWillDisappear:animated];
}

- (void)viewDidUnload
{
	[super viewDidUnload];
	
    if (point_program) {
        glDeleteProgram(point_program);
        point_program = 0;
    }
    
    if (line_program) {
        glDeleteProgram(line_program);
        line_program = 0;
    }
    
    // Tear down context.
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
	self.context = nil;	
}

- (NSInteger)animationFrameInterval
{
    return animationFrameInterval;
}

- (void)setAnimationFrameInterval:(NSInteger)frameInterval
{
    /*
	 Frame interval defines how many display frames must pass between each time the display link fires.
	 The display link will only fire 30 times a second when the frame internal is two on a display that
     refreshes 60 times a second. The default frame interval setting of one will fire 60 times a second
     when the display refreshes at 60 times a second. A frame interval setting of less than one results
     in undefined behavior.
	 */
    if (frameInterval >= 1) {
        animationFrameInterval = frameInterval;
        
        if (animating) {
            [self stopAnimation];
            [self startAnimation];
        }
    }
}

- (void)startAnimation
{
    if ( !animating )
    {
        CADisplayLink *aDisplayLink = [[UIScreen mainScreen] displayLinkWithTarget:self selector:@selector(drawFrame)];
        [aDisplayLink setFrameInterval:animationFrameInterval];
        [aDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        self.displayLink = aDisplayLink;
        
        animating = TRUE;
    }
}

- (void)stopAnimation
{
    if ( animating )
    {
        [self.displayLink invalidate];
        self.displayLink = nil;
        animating = FALSE;
    }
}

- (void)drawFrame
{
    [(EAGLView *)self.view setFramebuffer];
    
    qb_cuboid_update ();
    
    time_elapsed = -(float)[epoch timeIntervalSinceNow];
    
    static int fps = 0;
    static float last_time = 0;
    static float second_past = 0;
    dt = time_elapsed - last_time;
    last_time = time_elapsed;
    ++fps;
    second_past += dt;
    if (second_past > 1)
    {
        second_past = second_past - 1;
        char fps_text[64];
        snprintf(fps_text, 64, "FPS: %i", fps);
        statusBar.text = [NSString stringWithUTF8String:fps_text];
        fps = 0;
    }
    
    glDisable ( GL_DITHER );
    glDisable ( GL_BLEND );
    glDisable ( GL_STENCIL_TEST );
    glDisable ( GL_TEXTURE_2D );
    
    glEnable ( GL_DEPTH_TEST );
    glEnable ( GL_CULL_FACE );
    //glClearColor ( 1, 1, 1, 1 );
    glClearColor ( 80/255.0f, 150/255.0f, 250/255.0f, 1 );
    
    glClear ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    qb_world_render ( world_ctx );
    
    glClear ( GL_DEPTH_BUFFER_BIT );
    qb_model_render ( model_ctx );
    
    glClear ( GL_DEPTH_BUFFER_BIT );
    qb_tools_render ( tools_ctx );
    
    [(EAGLView *)self.view presentFramebuffer];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    
    touchMoved = FALSE;
    touchedGamut = NULL;
    touchedWorld = NULL;
    
    if ( touches.count == 1 )
    {
        UITouch* touch = [touches anyObject];
        CGPoint currPos = [touch locationInView:self.view];
        vec3_t screen_pos = { currPos.x, currPos.y, 0 };
        
        qb_tools_touch_began ( tools_ctx, screen_pos );
        
//        octant_t* octant = 0;
//        if ( qb_pick_select ( world_ctx, screen_pos, &octant ) )
//        {
//            touchedWorld = octant;
//        }
//        else if ( qb_pick_select ( gamut_ctx, screen_pos, &octant ) )
//        {
//            touchedGamut = octant;
//        }
    }
}

CGFloat CGPointDist(CGPoint point1,CGPoint point2)
{
    CGFloat dx = point2.x - point1.x;
    CGFloat dy = point2.y - point1.y;
    return sqrt(dx*dx + dy*dy );
};

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    touchMoved = TRUE;
    
    if ( touches.count == 1 )
    {
        UITouch* touch = [touches anyObject];
        CGPoint prevPos = [touch previousLocationInView:self.view];
        CGPoint currPos = [touch locationInView:self.view];
        //vec3_t prev_screen_pos = { prevPos.x, prevPos.y, 0 };
        vec3_t screen_pos = { currPos.x, currPos.y, 0 };
        
        qb_tools_touch_moved ( tools_ctx, screen_pos );
        
        if ( touchedWorld )
        {
            //qb_tools_touch_moved ( tools_ctx, screen_pos );
            //qb_tools_on_swipe ( tools_ctx, prev_screen_pos, screen_pos );
//            octant_t* octant = 0;
//            if ( qb_pick_select ( world_ctx, screen_pos, &octant ) )
//            {
//                qb_tools_on_tap ( tools_ctx, screen_pos );
//                //qb_tools_world_select ( tools_ctx, octant );
//            }
        }
        else if ( touchedGamut )
        {
            touchedGamut->rotation[0] += ( prevPos.y - currPos.y ) * dt * 20;
            if ( touchedGamut->rotation[0] < -60 )
                touchedGamut->rotation[0] = -60;
            if ( touchedGamut->rotation[0] > 60 )
                touchedGamut->rotation[0] = 60;
            touchedGamut->rotation[1] -= ( prevPos.x - currPos.x ) * dt * 20;
        }
        else
        {
            
        }
    }
    else if ( touches.count == 2 )
    {
        if ( true )
        {
            UITouch* touch0 = [[touches allObjects] objectAtIndex:0];
            UITouch* touch1 = [[touches allObjects] objectAtIndex:1];
            
            CGPoint touch0_ppos = [touch0 previousLocationInView:self.view];
            CGPoint touch0_cpos = [touch0 locationInView:self.view];
            
            CGPoint touch1_ppos = [touch1 previousLocationInView:self.view];
            CGPoint touch1_cpos = [touch1 locationInView:self.view];
            
            vec_t touch_pdist = CGPointDist(touch0_ppos, touch1_ppos);
            vec_t touch_cdist = CGPointDist(touch0_cpos, touch1_cpos);
            
            vec3_t touch0_dir = { touch0_cpos.x - touch0_ppos.x, touch0_cpos.y - touch0_ppos.y, 0 };
            vec3_t touch1_dir = { touch1_cpos.x - touch1_ppos.x, touch1_cpos.y - touch1_ppos.y, 0 };
            
            VectorNormalize ( touch0_dir, touch0_dir );
            VectorNormalize ( touch1_dir, touch1_dir );
            
            vec_t dot = DotProduct ( touch0_dir, touch1_dir );
            
            if ( dot > 0.9f )
            {
                vec3_t rotation;
                
                rotation[0] = ( touch0_ppos.y - touch0_cpos.y ) * dt * 15;
                rotation[1] = ( touch0_cpos.x - touch0_ppos.x ) * dt * 15;
                rotation[2] = 0;
                //rotation[0] = 0;
                qb_camera_rotate ( world_ctx, rotation );
                
                rotation[0] = ( touch0_ppos.y - touch0_cpos.y ) * dt * 15;
                rotation[1] = ( touch0_cpos.x - touch0_ppos.x ) * dt * 15;
                rotation[2] = 0;
                qb_camera_rotate ( model_ctx, rotation );
            }
            else if ( dot < -0.8f )
            {
                vec_t world_vel = fabsf ( world_ctx->ortho_aabb.extents[0] - 1 ) / 8;
                vec_t model_vel = fabsf ( world_ctx->ortho_aabb.extents[0] - 1 ) / 4;
                world_ctx->ortho_aabb.extents[0] -= ( touch_cdist - touch_pdist ) * dt * world_vel;
                model_ctx->ortho_aabb.extents[0] -= ( touch_cdist - touch_pdist ) * dt * model_vel;
                if ( model_ctx->ortho_aabb.extents[0] < 1 )
                {
                    model_ctx->ortho_aabb.extents[0] = 1;
                }
            }
        }
    }
    else if ( touches.count == 3 )
    {
        
        UITouch* touch = [touches anyObject];
        CGPoint prevPos = [touch previousLocationInView:self.view];
        CGPoint currPos = [touch locationInView:self.view];
        
        vec3_t front = { world_ctx->view_mat[2], 0, world_ctx->view_mat[10] };
        VectorNormalize( front, front );
        
        vec3_t right = { world_ctx->view_mat[0], 0, world_ctx->view_mat[8] };
        VectorNormalize( right, right );
        
        float dy = ( prevPos.y - currPos.y ) * dt;
        float dx = ( prevPos.x - currPos.x ) * dt;
        
        world_ctx->camera_target[2] += front[2] * dy;
        world_ctx->camera_target[0] += front[0] * dy;
        
        world_ctx->camera_target[2] += right[2] * dx;
        world_ctx->camera_target[0] += right[0] * dx;
        
        //model_ctx->camera_target[2] = world_ctx->camera_target[2];
        //model_ctx->camera_target[0] = world_ctx->camera_target[0];
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    
    if ( touches.count == 1 )
    {
        UITouch* touch = [touches anyObject];
        CGPoint currPos = [touch locationInView:self.view];
        vec3_t screen_pos = { currPos.x, currPos.y, 0 };
        
        qb_tools_touch_ended ( tools_ctx, screen_pos );
        
//        if ( touchedGamut && !touchMoved )
//        {
//            qb_tools_on_gamut ( tools_ctx, screen_pos );
//        }
//        else if ( !touchMoved )
//        {
//            qb_tools_on_tap ( tools_ctx, screen_pos );
//        }
    }
    
    qb_world_zoom_end ( world_ctx );
    
    qb_model_zoom_end ( model_ctx );
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source)
    {
        NSLog(@"Failed to load vertex shader");
        return FALSE;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0)
    {
        glDeleteShader(*shader);
        return FALSE;
    }
    
    return TRUE;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    
    glLinkProgram(prog);
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0)
        return FALSE;
    
    return TRUE;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;
    
    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0)
        return FALSE;
    
    return TRUE;
}

- (BOOL)loadPointShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    point_program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"assets/point" ofType:@"vert"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname])
    {
        NSLog(@"Failed to compile vertex shader");
        return FALSE;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"assets/point" ofType:@"frag"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname])
    {
        NSLog(@"Failed to compile fragment shader");
        return FALSE;
    }
    
    // Attach vertex shader to program.
    glAttachShader(point_program, vertShader);
    
    // Attach fragment shader to program.
    glAttachShader(point_program, fragShader);
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation( point_program, ATTRIB_POSITION, "position" );
    glBindAttribLocation( point_program, ATTRIB_COLOR, "color" );
    
    // Link program.
    if (![self linkProgram:point_program])
    {
        NSLog(@"Failed to link program: %d", point_program);
        
        if (vertShader)
        {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader)
        {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (point_program)
        {
            glDeleteProgram(point_program);
            point_program = 0;
        }
        
        return FALSE;
    }
    
    qb_point_prog = point_program;
    qb_gl_uniforms[UNIFORM_MVP_MAT]  = glGetUniformLocation ( point_program, "mvp_mat" );
    qb_gl_uniforms[UNIFORM_POINT_SIZE]= glGetUniformLocation ( point_program, "point_size" );
    qb_gl_uniforms[UNIFORM_POINT_TEXTURE] = glGetUniformLocation ( point_program, "point_texture" );
    
    // Release vertex and fragment shaders.
    if ( vertShader )
        glDeleteShader ( vertShader );
    if ( fragShader )
        glDeleteShader ( fragShader );
    
    NSString* path = @"assets/point";
    path = [[NSBundle mainBundle] pathForResource:path ofType:@"png"];
    NSData *texData = [[NSData alloc] initWithContentsOfFile:path];
    UIImage *image = [[UIImage alloc] initWithData:texData];
    if (image != nil)
    {
        // Get Image size
        GLuint width = CGImageGetWidth(image.CGImage);
        GLuint height = CGImageGetHeight(image.CGImage);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        // Allocate memory for image
        void *imageData = malloc( height * width * 4 );
        CGContextRef imgcontext = CGBitmapContextCreate(
                                                        imageData, width, height, 8, 4 * width, colorSpace,
                                                        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big );
        CGColorSpaceRelease( colorSpace );
        CGContextClearRect( imgcontext,
                           CGRectMake( 0, 0, width, height ) );
        CGContextTranslateCTM( imgcontext, 0, height - height );
        CGContextDrawImage( imgcontext,
                           CGRectMake( 0, 0, width, height ), image.CGImage );
        
        // use imageData
        glGenTextures( 1, &qb_point_tex_gl_id );
		glBindTexture( GL_TEXTURE_2D, qb_point_tex_gl_id );
        glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData );
        
        // Release context
        CGContextRelease(imgcontext);
        // Free Stuff
        free(imageData);
        [image release];
        [texData release];
    }

    return TRUE;
}


- (BOOL)loadLineShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    line_program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"assets/line" ofType:@"vert"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname])
    {
        NSLog(@"Failed to compile vertex shader");
        return FALSE;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"assets/line" ofType:@"frag"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname])
    {
        NSLog(@"Failed to compile fragment shader");
        return FALSE;
    }
    
    // Attach vertex shader to program.
    glAttachShader(line_program, vertShader);
    
    // Attach fragment shader to program.
    glAttachShader(line_program, fragShader);
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation( line_program, ATTRIB_POSITION, "position" );
    
    // Link program.
    if (![self linkProgram:line_program])
    {
        NSLog(@"Failed to link program: %d", line_program);
        
        if (vertShader)
        {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader)
        {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (line_program)
        {
            glDeleteProgram(line_program);
            line_program = 0;
        }
        
        return FALSE;
    }
    
    qb_line_prog = line_program;
    qb_gl_uniforms[UNIFORM_MVP_MAT]  = glGetUniformLocation ( line_program, "mvp_mat" );
    qb_gl_uniforms[UNIFORM_LINE_COLOR]= glGetUniformLocation ( line_program, "line_color" );
    
    // Release vertex and fragment shaders.
    if ( vertShader )
        glDeleteShader ( vertShader );
    if ( fragShader )
        glDeleteShader ( fragShader );
    
    return TRUE;
}

- (BOOL)loadSolidShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    solid_program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"assets/solid" ofType:@"vert"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname])
    {
        NSLog(@"Failed to compile vertex shader");
        return FALSE;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"assets/solid" ofType:@"frag"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname])
    {
        NSLog(@"Failed to compile fragment shader");
        return FALSE;
    }
    
    // Attach vertex shader to program.
    glAttachShader(solid_program, vertShader);
    
    // Attach fragment shader to program.
    glAttachShader(solid_program, fragShader);
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation( solid_program, ATTRIB_POSITION, "position" );
    glBindAttribLocation( solid_program, ATTRIB_UV, "uv" );
    
    // Link program.
    if (![self linkProgram:solid_program])
    {
        NSLog(@"Failed to link program: %d", solid_program);
        
        if (vertShader)
        {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader)
        {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (solid_program)
        {
            glDeleteProgram(solid_program);
            solid_program = 0;
        }
        
        return FALSE;
    }
    
    qb_solid_prog = solid_program;
    qb_gl_uniforms[UNIFORM_MVP_MAT]  = glGetUniformLocation ( solid_program, "mvp_mat" );
    qb_gl_uniforms[UNIFORM_SOLID_COLOR]= glGetUniformLocation ( solid_program, "solid_color" );
    qb_gl_uniforms[UNIFORM_SOLID_TEXTURE] = glGetUniformLocation ( solid_program, "solid_texture" );
    
    // Release vertex and fragment shaders.
    if ( vertShader )
        glDeleteShader ( vertShader );
    if ( fragShader )
        glDeleteShader ( fragShader );
    
    NSString* path = @"assets/solid";
    path = [[NSBundle mainBundle] pathForResource:path ofType:@"png"];
    NSData *texData = [[NSData alloc] initWithContentsOfFile:path];
    UIImage *image = [[UIImage alloc] initWithData:texData];
    if (image != nil)
    {
        // Get Image size
        GLuint width = CGImageGetWidth(image.CGImage);
        GLuint height = CGImageGetHeight(image.CGImage);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        // Allocate memory for image
        void *imageData = malloc( height * width * 4 );
        CGContextRef imgcontext = CGBitmapContextCreate(
                                                        imageData, width, height, 8, 4 * width, colorSpace,
                                                        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big );
        CGColorSpaceRelease( colorSpace );
        CGContextClearRect( imgcontext,
                           CGRectMake( 0, 0, width, height ) );
        CGContextTranslateCTM( imgcontext, 0, height - height );
        CGContextDrawImage( imgcontext,
                           CGRectMake( 0, 0, width, height ), image.CGImage );
        
        // use imageData
        glGenTextures( 1, &qb_solid_tex_gl_id );
		glBindTexture( GL_TEXTURE_2D, qb_solid_tex_gl_id );
        glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData );
        
        // Release context
        CGContextRelease(imgcontext);
        // Free Stuff
        free(imageData);
        [image release];
        [texData release];
    }
    
    return TRUE;
}

GLuint qb_load_texture ( const char* asset )
{
    GLuint gl_id = 0;
    
    NSString* path = [NSString stringWithCString:asset encoding:NSASCIIStringEncoding];
    path = [[NSBundle mainBundle] pathForResource:path ofType:@"png"];
    NSData *texData = [[NSData alloc] initWithContentsOfFile:path];
    UIImage *image = [[UIImage alloc] initWithData:texData];
    if (image != nil)
    {
        // Get Image size
        GLuint width = CGImageGetWidth(image.CGImage);
        GLuint height = CGImageGetHeight(image.CGImage);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        // Allocate memory for image
        void *imageData = malloc( height * width * 4 );
        CGContextRef imgcontext = CGBitmapContextCreate(
                                                        imageData, width, height, 8, 4 * width, colorSpace,
                                                        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big );
        CGColorSpaceRelease( colorSpace );
        CGContextClearRect( imgcontext,
                           CGRectMake( 0, 0, width, height ) );
        CGContextTranslateCTM( imgcontext, 0, height - height );
        CGContextDrawImage( imgcontext,
                           CGRectMake( 0, 0, width, height ), image.CGImage );
        
        // use imageData
        glGenTextures( 1, &gl_id );
		glBindTexture( GL_TEXTURE_2D, gl_id );
        glTexImage2D ( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData );
        
        // Release context
        CGContextRelease(imgcontext);
        // Free Stuff
        free(imageData);
        [image release];
        [texData release];
    }
    
    return gl_id;
}

qube_t* qb_qube_from_image ( const char* path )
{
    return [qubitorViewController qubeFromImage:[NSString stringWithCString:path encoding:NSASCIIStringEncoding]];
}

+ (qube_t*)qubeFromImage:(NSString*)path
{
    qube_t* qube = NULL;
 	path = [[NSBundle mainBundle] pathForResource:path ofType:@"png"];
    NSData *texData = [[NSData alloc] initWithContentsOfFile:path];
    UIImage *image = [[UIImage alloc] initWithData:texData];
    if (image != nil)
    {
        // Get Image size
        GLuint width = CGImageGetWidth(image.CGImage);
        GLuint height = CGImageGetHeight(image.CGImage);
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        // Allocate memory for image
        void *imageData = malloc( height * width * 4 );
        CGContextRef imgcontext = CGBitmapContextCreate(
                                                        imageData, width, height, 8, 4 * width, colorSpace,
                                                        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big );
        CGColorSpaceRelease( colorSpace );
        CGContextClearRect( imgcontext,
                           CGRectMake( 0, 0, width, height ) );
        CGContextTranslateCTM( imgcontext, 0, height - height );
        CGContextDrawImage( imgcontext,
                           CGRectMake( 0, 0, width, height ), image.CGImage );
        
        // use imageData
        qube = qb_qube_from_texels ( 0, imageData, height / 16 );
        
        // Release context
        CGContextRelease(imgcontext);
        // Free Stuff
        free(imageData);
        [image release];
        [texData release];
    }
    
    return qube;
}

@end
