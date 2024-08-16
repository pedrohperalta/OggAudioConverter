#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface OggAudioConverter : NSObject

- (void)convertOggToWav:(NSString *)oggFilePath outputFilePath:(NSString *)outputFilePath;

@end

NS_ASSUME_NONNULL_END
