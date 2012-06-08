/*
 *  QTAtom-ProgmCreation.c
 *  QTMovieSink-106
 *
 *  Created by René J.V. Bertin on 20120608.
 *  Code by Pierre Chatelier
 *
 */


QTAtomContainer atomContainer = 0;
error = QTNewAtomContainer(&atomContainer);
QTAtom tiffEndianAtom = 0; //not really useful, it will be false anyway. But I tried to reproduce the exact settings as given by the SCDialog
__int32 tiffEndianAtomValue = 0;
error = QTInsertChild(atomContainer, kParentAtomIsContainer, kQTTIFFLittleEndian, 0, 0, sizeof(tiffEndianAtomValue), &tiffEndianAtomValue, &tiffEndianAtom);
QTAtom tiffCompressionAtom = 0;
__int32 tiffCompressionAtomValue = convertHostToBigEndian32Bits(kQTTIFFCompression_PackBits);
error = QTInsertChild(atomContainer, kParentAtomIsContainer, kQTTIFFCompressionMethod, 0, 0, sizeof(tiffCompressionAtomValue), &tiffCompressionAtomValue, &tiffCompressionAtom);
error = (error != noErr) ? error :
ICMCompressionSessionOptionsSetProperty(options,
								kQTPropertyClass_ICMCompressionSessionOptions,
								kICMCompressionSessionOptionsPropertyID_CompressorSettings,
								sizeof(atomContainer), &atomContainer);
QTDisposeAtomContainer(atomContainer);
