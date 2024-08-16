#import "OggAudioConverter.h"

@implementation OggAudioConverter

- (void)convertOggToWav:(NSString *)oggFilePath outputFilePath:(NSString *)outputFilePath {
    NSLog(@"Converting %@ to %@", oggFilePath, outputFilePath);
}

@end
