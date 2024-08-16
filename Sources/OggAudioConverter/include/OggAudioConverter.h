#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface OggAudioConverter : NSObject

- (BOOL)decode:(NSURL *)oggFile into:(NSURL *)outputFile;

@end

NS_ASSUME_NONNULL_END
