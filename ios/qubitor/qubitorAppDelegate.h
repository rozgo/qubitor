#import <UIKit/UIKit.h>

@class qubitorViewController;

@interface qubitorAppDelegate : NSObject <UIApplicationDelegate>

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet qubitorViewController *viewController;

@end
