# Phatch - Photo Batch Processor
# Copyright (C) 2007-2008 www.stani.be
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see http://www.gnu.org/licenses/
#
# Phatch recommends SPE (http://pythonide.stani.be) for editing python files.

# Embedded icon is taken from:
# http://www.iconlet.com/info/9657_colorscm_128x128
# Icon author: Everaldo Coelho
# Icon license: LGPL

# Follows PEP8

from core import models
from lib.reverse_translation import _t

OPTIONS = [
    _t('Value'),
    _t('Top Left'),
    _t('Top Right'),
    _t('Bottom Left'),
    _t('Bottom Right')]

#---PIL


def init():
    global Image, ImageOps, ImageMath, imtools
    import Image
    import ImageOps
    import ImageMath
    from lib import imtools
    global HTMLColorToRGBA
    from lib.colors import HTMLColorToRGBA


def difference1(source, color):
    """When source is bigger than color"""
    return (source - color) / (255.0 - color)


def difference2(source, color):
    """When color is bigger than source"""
    return (color - source) / color


def color_to_alpha(image, color_value=None, select_color_by=None):
    image = image.convert('RGBA')
    width, height = image.size
    select = select_color_by
    color = color_value
    if select == OPTIONS[0]:
        color = HTMLColorToRGBA(color, 255)
    elif select == OPTIONS[1]:
        color = image.getpixel((0, 0))
    elif select == OPTIONS[2]:
        color = image.getpixel((width - 1, 0))
    elif select == OPTIONS[3]:
        color = image.getpixel((0, height - 1))
    elif select == OPTIONS[4]:
        color = image.getpixel((width - 1, height - 1))
    else:
        return image

    if color[3] == 0:
        # The selected color is transparent
        return image

    color = map(float, color)
    img_bands = [band.convert("F") for band in imtools.split(image)]

    # Find the maximum difference rate between source and color.
    # I had to use two difference functions because ImageMath.eval
    # only evaluates the expression once.
    alpha = ImageMath.eval(
        """float(
            max(
                max(
                    max(
                        difference1(red_band, cred_band),
                        difference1(green_band, cgreen_band)
                    ),
                    difference1(blue_band, cblue_band)
                ),
                max(
                    max(
                        difference2(red_band, cred_band),
                        difference2(green_band, cgreen_band)
                    ),
                    difference2(blue_band, cblue_band)
                )
            )
        )""",
        difference1=difference1,
        difference2=difference2,
        red_band=img_bands[0],
        green_band=img_bands[1],
        blue_band=img_bands[2],
        cred_band=color[0],
        cgreen_band=color[1],
        cblue_band=color[2])

    # Calculate the new image colors after the removal of the selected color
    new_bands = [
        ImageMath.eval(
            "convert((image - color) / alpha + color, 'L')",
            image=img_bands[i],
            color=color[i],
            alpha=alpha)
        for i in xrange(3)]

    # Add the new alpha band
    new_bands.append(ImageMath.eval(
        "convert(alpha_band * alpha, 'L')",
        alpha=alpha,
        alpha_band=img_bands[3]))

    return Image.merge('RGBA', new_bands)

#---Phatch


class Action(models.Action):
    label = _t('Color to Alpha')
    author = 'Nadia Alramli'
    email = 'mail@nadiana.com'
    cache = False
    init = staticmethod(init)
    pil = staticmethod(color_to_alpha)
    version = '0.1'
    tags = [_t('color')]
    __doc__ = _t('Make selected color transparent')

    def get_relevant_field_labels(self):
        """If this method is present, Phatch will only show relevant
        fields.

        :returns: list of the field labels which are relevant
        :rtype: list of strings

        .. note::

            It is very important that the list of labels has EXACTLY
            the same order as defined in the interface method.
        """
        relevant = ['Select Color By']
        if self.get_field_string('Select Color By') == OPTIONS[0]:
            relevant.append('Color Value')
        return relevant

    def interface(self, fields):
        fields[_t('Select Color By')] = self.ChoiceField(OPTIONS[0],
            choices=OPTIONS)
        fields[_t('Color Value')] = self.ColorField('#FFFFFF')

    icon = \
'x\xda\x01o\x0c\x90\xf3\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x000\x00\
\x00\x000\x08\x06\x00\x00\x00W\x02\xf9\x87\x00\x00\x00\x04sBIT\x08\x08\x08\
\x08|\x08d\x88\x00\x00\x0c&IDATh\x81\xd5\x9a\x7fp\x94u~\xc7_\xcf\xcf\xec>\
\xfb#\xc9&\x1b\xb21\x81B\xa0\xf2\xcb\xca\x8f\xa6B-\xa7\x1et\xe4r\x1e\xf2\xa3\
\xeaa9\x83\x9d3\x03XGph;7Nu\xa4\xd3\xe2\xe9\xc0\x0c\xe7X\x1d\xf4\xec\x1cZ\
\xb5\x1eg\xe9\xb4\xcaq\xd7\xe9\xe9M\x0e\x0esp\x9cB\x01\x03&\x90\x98M\x96M\
\xc8fw\xb3?\x9e_\xfd\xe3\xd9\xddl\xc8b\x85\xa0x\x9f\x99\xefL\xf6y6\xdf\xe7\
\xfd\xfe|\xde\x9f\xcf\xf7\xf3\xfd\xee#\xd8\xb6\xcd\xef\xb3\x89\xd7\x1b\xc0DM\
\xfe2\x1f&\x08\x82p\xe95{\x82\x12\x10\xbeh\t\xe5@\xe7\x87\x08H\xb9\xbfm\xc0\
\x04L\xdb\xb6\xad\xab\x9d\xff\x0b\x8b@\x0ex\x1e\xb0\x02\xa8@Yn\xc8\xabW\xaf\
\x0e,Z\xb4h\xdaK/\xbd\xb4O\x10\x84\xec\xd5F\xe2\x9a\x13\xc8\x01\x97rs\x97\
\x01\x1e\xc0\xbbh\xd1\xa2\xc6e\xcb\x96\xad\xac\xa9\xa9i\xf2x<\x8d\xb2,\xfbu]\
\xef\xe8\xe8\xe8x\x07\xd0q"r\xc5v\xcd\x08\x14y\\\x01\xdc\x80\xbf\xaa\xaa\xaa\
\xa6\xa5\xa5eucccsee\xe5\\@\xb4m\x9b\xfc\x18\x1e\x1e\xfe\r\xa3\x92\xba*\x9b0\
\x81"\x8d+\x80\x0b\xf0\x01\xc1\x87\x1f~x\xf5-\xb7\xdc\xd2\xeav\xbbk\x8aA\x17\
\x8fH$\xd2\x0eX\\\xa5\xf7\'D\xa0\x08\xb8\x84#\x15\x1f\x10X\xb3f\xcd\xd7\x9a\
\x9b\x9b\x1f\xad\xaa\xaa\xba\xf1r\xc0m\xdb\xc6\xb2,\xfb\xfd\xf7\xdfo\xc3I\
\xe4/\x97@\x91\xceU\x1c\x8dW.^\xbcx\xce\xbau\xeb\x1e\x9d2e\xca\xd7l\xdb\x16>\
\x0b\xbcm\xdb\xa4R\xa9\xaeC\x87\x0e\x85s\x04\xae\xda\xae\x88@\t\x9dW\x84B\
\xa1\x86\xcd\x9b7o\x983g\xcejI\x92\xcaJ\x81\x05\xb8\x90\xb6\xd1$\x1b\x97\xe4\
\\\xcbd2\x1d@\x06\xa7\x8c~\xb1\x11\xb8D..\xc0\x0f\x04\xb7l\xd9\xb2\xea\x8e;\
\xeexX\xd3\xb4\xaa\xcby\xbaw\xc4\xe2\xc8\x80E_\xca\xa6e\xaa\x88@!\x02\x9dL\
\xa0\xfa|n\x02%\xe4R\xb5t\xe9\xd2\x9b\x1f|\xf0\xc1\xbf\xad\xaf\xaf_p9\xe0\
\x9d\xc3\x06\xed\x17\x0czG\x9c5jf\x85\x84,\t\x85\xfb\xf1x\xfc\x0c\x13\xd4\
\xffg\x12(\x92\x8bLN.>\x9f\xafn\xdb\xb6m\x1b\x16.\\x\x9f(\x8aj)\xe0\xe1\xa4\
\xc9\x7fw\xa7\xe9\x1bq\xa4-\x89\x02\x8a$\xf1\x87\x15\x12\x92D\xe1{\xfd\xfd\
\xfdgp*\xd0\x84\xac$\x01A\x10\xf2+h\xbe\xba\x047n\xdcx\xe7\xaaU\xab6\xfb\xfd\
\xfe\xbaR\xc0\xd3\x86\xc5\xfb=)~{!\x8bm\xdb\x88\x82\x80*\x8b(\x92\x84*\x8b\
\x844\x11Q\x1c\xfd~[[[\x07\xd7:\x02E^W\x01\r\x08455\xcd\xdc\xbau\xeb\xdfL\
\x9b6\xed\xcf\xc8\xf5N\xf92\x18\x0e\x87\x07C\xa1P\xd5\x89h\x9a\x9fu\xc5I\xea\
y\xe0R\x01\xbc"\x8bh\xb2\x84W\x1d\xf5~:\x9d\x1e8v\xec\xd8\x10\xd7*\x02EI\x9a\
\x97K\xb9\xa2(\xb5O?\xfd\xf4\xfa%K\x96\xb4\xa8\xaa\xaa]\xea\xf1\xfd\xfb\xf7\
\xbfs\xe2\xc4\x89\xde9KW,\x8bZ\xee\xaaj\xd1&\xe8\x12\x10%Y\xc9\xca\x8a;k\x8b\
B\x9eD\xd0%\x8c\xf1\xbe\xae\xeb=\xe4\x12\xf8J*\xd0\xa6M\x9b*\xb3\xd9l\x95\
\xdf\xef\xfft\xe7\xce\x9d)\x18\xed\x12\xf3I\xea\x03\xaa\xd7\xaf_\xbf\xa4\xa5\
\xa5ek0\x18l,%\x97\xc1\xc1\xc1\xf3\xcd\xcd\xcd\xad\x99L&\x9b#\x9d\xef}\xdc@\
\r0\xf9\xef~\xb4\x7fsF\xd6TE\x12\x99\xe2\x11\xb8\xc9o\x16\xff\xff\x07k\xd6\
\xaci\x06b\xb6m\x1b\x97\x03\xdc\xda\xda\xda\x04|C\x14\xc5\xf9\x8a\xa2,\x94$\
\xa9A\x10\x04\xd2\xe9t\xc2\xb6\xed\x96\x17_|\xf1m\x19G2.\xa0j\xd6\xacY3\x9ez\
\xea\xa9-7\xddt\xd3rA\x10\xc4R\xe0?\x8e\x8e\xd0\xfe\xbb\x8e\x9ff2\x99\x8f\
\x81T\x91\x03\x14\xa0\x02\xb0\xbeq_\xcb\xadV\x99W\xd5$\x11E\x96\xf0\x951&\
\x81eY\x06G\xfb\xe3\xbc\xbfq\xe3\xc6\x1bl\xdb^\'\x08B\x8b\xa6i\xb34M\xc3\xed\
v\x03`\x9a&\xc9d\x12\xcb\xb2\xbc\x82 \xac\x03\xde\x96s\x9e+ojj\xfa\xa3W^ye\
\x8f\xdb\xed\xae(\x05<\x96\xd2\xd9w2\xc2\xe9\x0bI\x8e\xbe\xb0\xe3\x05`\x00H\
\xe7\x9e+\x01^\xc0\xff\xc7K\xbe\xde\xb0\xf8\xde\xef\xdeg\x08J.\x0fD\x14\x05D\
\xd1*\xcc%\x8a\xe2\x18\xf0\x82 \x08\x0f=\xf4\xd0JA\x106\xb8\\\xaee\x15\x15\
\x15\xa2\xcf\xe7C\x10\x9c\xb2k\x9a&\x86a\x90H$\x18\x1e\x1e\x06G{\x1f\x90\x03\
\xaf\xe0h~\xaa\xd7\xeb\x1d\x07\xde\xb4,\xda\xba\x86x\xf7t\x04\xd3\xb2\xf9\
\x83\n\xf5\xe2\xbfwtDr\xe0\xf5\xa2\x08h\xd5\x93\xea\xa6\xdc\xfb\xd8\xb6\xefg\
DU\xd4d\x115\x17\x01Y\xb6\x91\xa4Q\tI\x92\x04\xc0\x9c9s\xa4\xd6\xd6\xd6o\xb7\
\xb6\xb6~\xaf\xbc\xbc|NMM\r^\xaf\x17\xcb\xb2\xb0,\x0b\xd34\x0b#\x16\x8b\x11\
\x8b\xc5\xf2\x9c/\x02?\xca\x13\x90\x00w:\x9d\xf6J\x924\x06|\xd7`\x927~\xdbC8\
\x9eA\x91$\xb42\x91r\xd18\x95\x03\x9f\xefa\xf2\x12\xac\xde\xf8\xf7\xff\xf8\
\xa8\xa5\xb8TY\xce\xf0\xa9p\x9a\xa4x\x81a3\xc2\\}\x16S]7\xe7\xab\x17\x99L\
\xa6v\xfb\xf6\xed\x87\xfb\xfa\xfa\xa6\xfa|>\xb9\xae\xae\x0e\xb7\xdb]\xb8o\
\x18FA2\x96e\x11\x8dF\x0b\x9e\xcfy\x7f\xd3\xee\xdd\xbb\xc3y\x02\x02 \xea\xba\
.\x8a\xa28\x86\xc0\xdb\x1f\x85\x89\x8e\xe8h\xaa\x82"\x8b\xa8\x92\x84\x90\x19\
ngl\x0b \xe2\x94\xdc\xaa\xf0\xe4\x9e\xa6C\xe2\xbf\xf0\xb1\xd1\x8e\x9e\xce84S\
\x90\xaaX\xcb\xb7\xca\xe7\x93J\xa5\xe8\xea\xea\xe2\xf0\xe1\xc3S\xfc~?\xf3\
\xe6\xcd\xc3\xe5r\x15\xc0\xe6\x01\xe7e\x93\xc9d\xe8\xed\xed%\x95J\x15\xa7\
\xc9\xbf\xed\xde\xbd\xfb\xcd\xfc\x87\xfc: \x98\xa6i\xe7\xf4H~\xef\xad\x95\
\xc9\x05\xf0\x8a$\xa1J"\xf1\xae\xde\xc3E\xde\xcf\xcb\xc7\r\x94\xef\x17\xf6x?\
\x8d\x9fu\x80\x17\x8d$C\xa4R)\xfa\xfb\xfb9~\xfc8s\xe7\xce%\x18\x0c"\x08\x02\
\x96e\xa1\xebz\x81\x84a\x18\xe8\xba\xce\xd0\xd0\x10\xe1p\x18\xd3\x1c\xd3\xac\
\x86M\xd3\xdcT|A\xccy\xd26\x0cc\xdc\xa2\xe2V\x94\x02\tM\x95\xd1\xca\x14\x8e\
\x1c\xfc\xc5I\xc6nB\xf2\r\x9e\xa7?\xda-0\x00\xc5C\x1a\x94\xd80\xa5\x85\xf9\
\xf3\xe7\x13\x89D\x08\x04\x02444PVV\x06P\x00\x9c\xc9d\n\xa3\xb7\xb7\x97\x9e\
\x9e\x1eL\xd3D\x10\x04DQD\x10\x84\x84eY\xab_~\xf9\xe5\xc1b\x8c\x85\x95\xb8$\
\x812\x19-\xa38\x95D\x96\xd0\x14\x89_\xfe\xf4?\xa3\x8c-\x7f"\xa04.\x9a5\xf9\
\xec\xc0i\xc7\xeb\xc3\xc0Y\x1bz`\xcb7\x1f\xa5y\xd1r\xc2\xe10\x83\x83\x83\xd4\
\xd5\xd5\x91\xcdf1\x0c\x83L&S\x00\xaf\xeb:\xf1x\x9cp8L:\x9dF\x14E\x14E\x01\
\xc0\xb2\xac\x94$I+v\xed\xda\xf5\xebK1\xe6\t\xd8\x86a\x8c\xab\xc9nEFSM\x14YB\
\x91D\xca]\xb2\t\x18\x8c\xb6\x00\x85\x15|\xf2\xcdSg\x9c\x1d>\r\xef\xd9\xf0+\
\x9b\xda`-\xa6ir\xf7\x1dw\x93L&1M\x13Q\x14\x91e\x99d2\x89\xae\xebd\xb3Yt]\'\
\x9dN\xd3\xdf\xdf\xcf\xd0\xd0\x10\xa2(\xa2\xaa*\xaa\xaa"I\x12\xa6ifEQ\xbc\
\xef\xd9g\x9f}\xefR|\x97F`\xdc\xce\xc8\xa5Hhe\xa3\x11\xf0\xaa\xa2\xc9\xf8\
\xfeE\x00\xc4Ty\xe2F\xf6\xd9x\xcej\xdc\xba\xf4VB\xa1\x10g\xce\x9c!\x10\x08\
\x10\x8f\xc71\x0c\x03\x97\xcbE4\x1aE\xd34t]G\xd7u\x06\x07\x07\x89F\xa3X\x96\
\x85,\xcb\x94\x95\x95\xe1v\xbb\x11\x04\x81L&c\xa8\xaa\xfa\xc0\xf6\xed\xdb\
\xdf\xe12M_\xfeh\xb1d\x0e\xb8\x14\xb9\xa0}]\x19\xe4\xe7\xfakR\xa9I\x80\x99G\
\x9e;4{\xf2P\x03\xb7\xddv\x1b===\x04\x83A\xb2\xd9,\xc9d\x92d2\xc9\xf0\xf00\
\x93&M\xa2\xaf\xaf\x8fD"\xc1\xe0\xe0 \x9f|\xf2\t\x91H\x04A\x10\xf0x<TVVRYY\
\x89\xa2(d\xb3\xd9\x8b\x8a\xa2\xac\xd9\xbe}\xfb\x8fK8\xadd\x04\xc61t)2Z\x19\
\x1c1\xf6\xf3jt\x1b\xa6aH\x15\xa1\n\xd7Px\xa8\xf8L\xf5/\x80\xe7ed\xd5\xe3\
\xf1\xd0\xd3\xd3\xc3\xda\xb5kI\xa7\xd3D"\x11\xe2\xf1x~\x03\x83a\x18\x04\x02\
\x01\xce\x9f?_(\x8d\x92$\xa1i\x1a~\xbf\x1fEQH\xa7\xd3\xe8\xba\xfe\xa9\xc7\
\xe3\xb9\xfd\xc9\'\x9f<s9\xe0\xa5\x08\x8cc)*\x06oF\xff\x81\xf7\x86\xf6:\x95\
\xdf\x80\xc5O6\xef\xdc\xbf\xe1\xed\x16\x9cR*\x00G\x81\x91\xda\xdaZu\xd5\xaaU\
444\xd0\xd7\xd7\xc7k\xaf\xbd\xc6\xb9s\xe78|\xf80\r\r\r\xc4b1.^\xbc\x88m\xdb\
\x04\x83A\xce\x9f?\xef<#\x97\x17\x00\xc9d\x12\x8f\xc730m\xda\xb4\x9b\x1fx\
\xe0\x81\x81\xff\x0f<9\x00~`\xba\xdb\xed\xfe\xd3\x8e\x8e\x8e\xe7\x8ao~\xe7\
\xc8&~q\xe1\xbf\x9c\xb4\xcd\x8d\xcat\x95]\xff\xaf\xb5\xd3?\xfa\xe8\xa3(P\xee\
\xf1x\xf6\xaeX\xb1\xe2O\xe6\xcd\x9bG4\x1a\xe5\xe0\xc1\x83\x1c;v\x0c\xd34I\
\xa7\xd3\xcc\x9e=\x9b\xb5k\xd7\x12\x8b\xc5\xe8\xed\xed\xa5\xbb\xbb\x9b\xfa\
\xfaz\x16,X@2\x99$\x1e\x8f\xa3\xaa*>\x9f\x8f\xea\xea\xea\xf3\xc1`\xf0\xb6\
\x95+Wv}\x1e\xf0c\x08(\x8a\xb2\xa8\xb3\xb3\xf3\xf9\xe2\x9b\xf7\xb6\xad\xe7`\
\xe4\x7f\x1c\xf0\xfa(\x89\x19\xf2\xcc\xec\x82\x9ey\x7f\xd9~\xe0\xd7\xf5\xb1X\
\xec\xfb\xd9lV\xcdf\xb3x<\x1e\xfc~?555\x1c?~\x9c\x91\x91\x11\x00\xee\xbc\xf3\
N\xdcn7\xdd\xdd\xddX\x96E(\x14\xa2\xb6\xb6\x96\xfa\xfaz\x02\x81\x00\x1e\x8f\
\x87\xf2\xf2\xf2\xddn\xb7{\xcb]w\xdd5\xf2y\xc1C\x91\x84t]\x1f\x97\x03\xaa\
\xa1@\x921\x11\x10t\x81\x99Sf\xa9O\x7f\xe7\x9f~<\xf2\xd7\xc9\xf0\xd1\xa3G\
\xa5\x9d;wr\xf2\xe4I.^\xbcH"\x91\xa0\xbb\xbb{\xcc\n\xfa\xe1\x87\x1f\xb2p\xe1\
Bf\xcc\x98\x81a\x18L\x9a4\x89\xea\xeaj\xdcn7n\xb7\xfb\x94\xc7\xe3\xf9\xde=\
\xf7\xdc\xb3\xefJ\x80_J\xc0&wT_|\x84\xaf\x18\n\x8c\x8c\x82\x9f\xa1\xdd\xc8+\
\xdf\xfe!S+\'\x03P^^\x1e\xba\xfd\xf6\xdbQ\x14\x85\xd7_\x7f\x9d\x9e\x9e\x1e\
\xe2\xf18\x89D\x82l6\x8b\xd7\xeb%\x10\x08\xa0i\x1a\x9d\x9d\x9d\xf8|>B\xa1\
\xd0\x99\xda\xda\xda\x9f\xd4\xd4\xd4\xf4z\xbd\xde3\xf7\xdf\x7f\xff\x01&p\xb8\
5fOlY\x96%IR\xa1\xc2(\x86\x0cI\x98?\xa9\x89\xef\xce\xff+\xee\x9e\xfd\xcdq\
\x13\x08\x82\xc0\xf4\xe9\xd3?ill|+\x9b\xcd\xdePQQ\xb14\x93\xc9\xd4\xe5\xef\
\xeb\xbaN"\x910\x81\x8e\xce\xce\xce\xa7\x0e\x1d:\xf4\xe6\xb8I&`\xc5\x04\xc6I\
\xe8\x06\xd7\r\xec\xf8\xf3\x1fp\xef\xdc\xbb\xc7D&o###\xf1w\xdf}\xf7\xd5\xad[\
\xb7\xbea\x18F\x18\x18Z\xbe|\xb9\xed\xf7\xfb=\x1d\x1d\x1d\xa1\xae\xae\xae\
\xcad2\x19\x01N\xd8\xb6\x9d\xbd\x96\xc0\xf3\x96O\xe2i\xc0-]]]\xcf\xcb\xb2|\
\xb9\xc5\xaa`\x96e\x99\xed\xed\xed?\x7f\xfc\xf1\xc7\xff\xf9\xd4\xa9SgqZ\xb7\
\x04\x90e\x82\xbf\xb8\\\xa9\x8d\x89\x80eY\x16NwyY;w\xee\xdc\xff\xee\xda\xb5\
\xeb\x07o\xbd\xf5\xd6\x07\xc0\x05\x9c\xd6-\x8d\x93)Wt\xcap-L\xa6hs\xfdY\xcf\
\x1e\x1e\x1e\x1e\xd8\xbbw\xef\x0f\x9fx\xe2\x89}@\x1f0\x84\xb3\xa9\xd7\x01\
\xeb\xcb\x06\x9e\xb71\x11(\x05\xc20\x0c\xbd\xad\xad\xed?\x1e{\xec\xb1\x97"\
\x91\xc8y`\x90\xb1r\xb9\xae?4\x17\x97\xd1q\xda=}\xfa\xf4o\x9ey\xe6\x99\xe7\
\x0e\x1c8\xf0;\x1c\xb9\xc4q\x8e\xc4\xaf\x8b\\JY^B&\x8e\x8e-\x80\x81\x81\x81\
\xde={\xf6\xbc\xb0c\xc7\x8e\x9f\x01\x11F\xe5bp\x1d\xe5R\xcad\x1c\xd0)\xe0\
\x82\xae\xeb\xb1\xb6\xb6\xb67\x1ey\xe4\x91W\x13\x89D/\xce\xf1E\x12G\xe7\xd7]\
.\xa5L\xc09R,\x07\xaa\x9a\x9a\x9a\x02\xed\xed\xed\x03@\x0cG\xe7\x19r\'\xc8_E\
\xf00\xf6\\\xb4,\xf7Y\xc7I\xd0\xaf\x8c\xce?\xcb.}\r\x00r\'\x0e_u\xe0y+\xbc+\
\x91\x7f\x11\xe3\xf7\x05x\xde\xfe\x0f\x12\xd6\x99\xed[\xf8N\xa2\x00\x00\x00\
\x00IEND\xaeB`\x82\xa4\xe7Gx'