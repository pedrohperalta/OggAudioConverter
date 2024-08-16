#import "OggAudioConverter.h"
#import "oggHelper.h"

@implementation OggAudioConverter

- (BOOL)decode:(NSURL *)oggFile into:(NSURL *)outputFile {
    const char *fileInChar = [[oggFile path] cStringUsingEncoding:NSASCIIStringEncoding];
    const char *fileOutChar = [[outputFile path] cStringUsingEncoding:NSASCIIStringEncoding];

    oggHelper helper;
    int output = helper.decode(fileInChar, fileOutChar);
    
    return output == 1;
}

@end
