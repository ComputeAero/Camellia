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

# Embedded icon is taken from www.openclipart.org (public domain)

# Follows PEP8

from core import models
from lib.reverse_translation import _t

#---PIL


def init():
    global ImageChops
    import ImageChops


def offset(image, horizontal_offset, vertical_offset=None):
    return ImageChops.offset(image, horizontal_offset, vertical_offset)

#---Phatch


class Action(models.Action):
    label = _t('Offset')
    author = 'Stani'
    email = 'spe.stani.be@gmail.com'
    init = staticmethod(init)
    pil = staticmethod(offset)
    version = '0.1'
    tags = [_t('transform'), _t('filter')]
    __doc__ = _t('Offset by distance and wrap around')

    def interface(self, fields):
        fields[_t('Horizontal Offset')] = self.PixelField('50%',
                                                choices=self.PIXELS)
        fields[_t('Vertical Offset')] = self.PixelField('50%',
                                                choices=self.PIXELS)

    def values(self, info):
        #pixel fields
        x, y = info['size']
        return super(Action, self).values(info,
            pixel_fields={'Horizontal Offset': x, 'Vertical Offset': y})

    icon = \
'x\xda\x01\xf5\x05\n\xfa\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00\x000\x00\
\x00\x000\x08\x06\x00\x00\x00W\x02\xf9\x87\x00\x00\x00\x04sBIT\x08\x08\x08\
\x08|\x08d\x88\x00\x00\x05\xacIDATh\x81\xed\xd8{\x8cTw\x15\x07\xf0\xcf\xcc\
\xec<vg\xb6T\x83B\n6\x8a/\xd2&H\x91\xc4\xc4\xd0\xb4$\xb5\xb4j\x1b\xeb\x83\
\xe0\xa3$\xa2\xa6m\x10\xa9\x94\xd2&>X\x15\x1f`R!\x9a&\xb8\xa9\x8d\x01$\xd2?J\
\x03\xad\xa6V\xe1\x8f*5\x8a\xa5\xb1T\x04E\xa5\x18\x9a\x96\xa5Ev\xe7\xb53s\
\xfd\xe3\xde\xbb\\`vvi\xfb\x97\xce79\xd9;w\xe7\x9es\xbe\xe7\x9c\xdf9\xe7\x0e\
]t\xd1E\x17]t\xd1E\x17\xff\xbfHM\xea[+\\\xa2\xcfZiW\xa1(\xf0\xa2\x96\x9d6\
\xf8\t\x82Wk\xfc>z{\xf8d\x8a\xebR\xcc\n8\x13\xf0T\xc0\xe0J\x8e\xbd>\x04\x06\
\xbcY\xda2\x19\xcb\x05f\x82\x16\x9ahZo\x9d{_\x8d\xf3\xf7\xf3\xee\x0c\x8f\xa5\
\x99\x95J8\xd2B@\xa5\xc5go\xe7\xe7\xaf\x9d\xc0\x06se<-}\x9e\x95\x06\x1a\x9a\
\xeaf\x1b\xf0\xb7\x8b%\xf0 \x8fg\xf8@\x1a\xe9\xc4\xfd\x96\xb1\xf8\x8c\x06\
\xcc]\xc6s\x9d\xf4\xa4;\xfd\x13\x14\x95\x14i+}2z}\xf0b\x9d\x87\x02\x97\x15\
\xd0;\xbed\xf3\xcc\x99HO\xcf\x84\x96r\x86\xe4\x05z\xa4\xc62\xd0\x14f\xa0\x8e\
\x9eI\xe8\xc0f\xfar,\xc9Q\xcbrG\x96+{\x90\xd1>\x03a\x82m\xd8N6\xc3\xc3CL\xbd\
\x83\x7f\x9e\xaf7~6e\x8d\x99\xda\x95\xd4\x17\xfcE\xbfu\x8a\xeaJ\x92\xd1\x0f\
\xa5\xe03\xbe\xaf8\x11\x81<\xab\xf3<\x90\xe7\xa7\x05\x9e\xc93/\xcf\x17{9\xde\
\x1b\xa9\x8a\xa3_\xe0x\x9e\xc59\xb6\xe4\xb9\xaf\xc5\x99>\xb6\x07m\xfc\x0bo\
\xac\xb1I\xca\x97\x14\x1dRt\xbd\xbb<\x7f\x81\x07O\xd8(m\xa5@\x98\x81QT\r\xa8\
(\xa8\xb8\xc6\x19K\xad\x1a\xff,leY\x9e\x1b\x0b\xec\xbc\x89m\xf1\xfd\xdf\xf3\
\x9e\x14\x07\x12\xc9}9\xc5;\xdf\xc7\x10<B\x7f\x93\xa7k\xbc\xd0\xe2\xa6O\xf3r\
;\x02{e]\xa3\x84\x92!E\xdb\x94|\xd7b/\x80\xdf\xe9\x95rL\xc6\xd4\xb1\x0eTC\
\xd5\xb3\x9e7O\xc1\x02eO([\xe2.\x0f\xb5#\xb0\x99\xec\x0c\xde\xd1\xc3\x15\x05\
\xae\xccrE>\xfc<;C1A h\xf0X\x9d}\xa3\xfc\xb5\xce\xe1&\x7f_\xc4H;\xbd\xe1s\
\xab]\xa7d\xab\x92i\x11\t\x8a*\x8a\x06\xf5\xd9\xa2\xe0cr\xee\x95\x11v\xfd0\
\xfaTPq\xbb\x0f\xd9l\xd0\xb5F\xdc\xe0\xce\xb0\xadn\xe2\xf2,\xeb\xf2\xf4\xf7R\
,pm\x81l\x01\x05\xe4\x91CVx\x10\x93\xc7\xab\x9e\x90j(\xad*\xff\xaa\xf2\x9b\n\
\x9b\xeb\xe4o\xe3I\x9255\xa0\xcf\x1b\xfcR\xd1\xd5J\xe8wn\xbd\x17"K\xadHs9\
\x92aG\x9d\xf6.\x8b5\x93\x91\xd9\xc4@\x8e\xb5\xc9\xce\x12;\x1f_\'I\xa4\x13\
\xb1\xa9G\t\xaeE1\xaa\x9e\x13\xaf\xb1\xebo\xaf\xe4\xabg\x1b\xc0\x80\xb2\xa6E\
\xd2\xee\x96uR&\xd2\x9c\x8b,\xf5&\x08\xc5\xd6\xc3\xf0\xcdR\xb0\xa0Mjw\xa49\
\x9af(\xcdsi\xf6\xa7y4\xcb`\x86\xf5=\xec\x89\xd5\x8fG*\xc3o3\xfc:\xc3\x914\
\xb5xfDQ\x9f}n\x06\x92\xd8\xea\x12\xd3\xacR\xb2J\xbf~\xfd\xc2\xb2\xea\x8bB4\
\x1c\xc9\x99H\xfec\xa3\x9b}\xb9\xad\xaeqp\x88\x9b\xfbx$v<\'l\x9dq\xa4\xab\
\x9cx;3R\x89Ue\x07\xd3\x1b\xbc\xad\xc2\xc2\x06\x0f\xde\xc6\x89\xceV\xfeh\xaa\
\x83~\xe8\x98\x96S\x02u\x813\x02\'\x04\x0e\x0b\xec\x17\xd8+\xb0\xdb?.\xc6y8\
\xcc\xfd\xc7\x08^"\x18&\x18%(\x13\x0c\x11\x1c\'8B\xfd`\x18\xb6\x8e\xe8<\x89\
\xe7;i\xd4n\r#c\x91\xaf\tC\xd5\x94\x9c\xfbo\xf5\xb3\xc9\xefD\x07\xc95\xb9%nh\
ua\xd4c\xd5\xd1>\x94\x1d\xe5\xfa\xd7F`\xafU\x9a\x1eUSR\x166\xb2\xb20\xcfua95\
\xc5\x93\xf9;~\xe4{\x06&\x9e\xcce\x964\x99>*\xee\xc6g\x0fg\xacv4$\xf2\x89\
\x89t\xb5?\x03\x03\xd2\xae\xf2cE\x9f\x1b\xeb@y\xe1\xdc\x8f\xe7|5"3"\xccL\xfc\
w\xd8\x01\xa7-\xb5\xde\x9f\xc73\xba\x8f\xfdy\xe6\xe5\x84\xb5\x1f\xb7\xd1\xe4\
\x8c\x8cZ\xe8p\x857-\x0c\xad\xb5E\xfb\x0c\xcct\xa9\x9a\xab\xcf\xe9]IG\x93\
\x99\xa89\x1b\xb2P\xe6\xe2Iw\xba\xb4\x9d\xea\xc7\x99_g^\xdc*\x93&\x92-3R[j\
\xb0h<\xe7\xc7\'\xf0y\xa7\xf0^5_S1<\x16\xe9\xa4\xf3\xb1\xc5ZBF55<\xace\xa1\
\x8d^i\xa7\xba\xceG\xc6s\xbe\x1d\x89:7v"0~\xbd.6\x8cuv8\xaaa\xdb9\xe32Z\x15\
\xc7\xbc\xa8\x8e]o\xf7-\xb7v2\xd8`\x17\x9ei\xd1\xdf\xc3e=\xacN3%.\xa1\xa8/\
\xecipd\x94\x97\xea\x1c\xea\xa4\xaf\xf3\x0b\xcd\x0e\x19e\x7f\x925\'\xda{wiX\
\xa6G\xde\x88\xac\x8a\xd3Zf\xab\xd9\xad\xe2\x8d\xcah\xf8\x86\r\x06:\xeaM\xe0\
\x17<\x94\xe1\xe3\xf1\xe7\x16\'\x9fb\xda@\xc8eB\\\x98\x81\xaf\x98!\xe7\xfd\
\xf2R\xfe\xed\xc3\xf2\xe6\x08\x17\xf7gU}\xcar\xc3\xe7=\xb1\xcf7\xdd\xa0\xeeW\
\x1a\xa6`\xad5\x0e\xd8`\xe7d\x1c\xa8Eo\\\x89\x97\xbdC\x93u\x9evg\xa0aX\xd9|\
\x15\x83*n\x8d\xea\xfe\xb4\xaa[\xda8\x1f\xe2\xeb\xfe\xa0fu\xe2\xce\xea\xb6\
\xdfk\x83\n/\xc6\xb5\x1f\x1d\xad\x0bW\xf9\x8b"\xb0\xdei\xeb\xdd\xe3\x94\xb7\
\x18\xb1\xd2\x88]\x86}\xd4\xf2\x0e\xef\xbd\xf7\x98"=\xb6J\xec\x91\xb6t\xb2\
\x0e\x94\x99r\xde\x01\xae_\x0c\x81\xc9\xfd\xac2\x11\xee\xb6E\xca\xe5\xf8\xc1\
dK\x076\xb1 \xc3`\x8a\xe9\xc2\xfd\xf7\x95V\xb8\xdf\xacX\xc1\xde\xd7\xc5\xb7.\
\xba\xe8\xa2\x8b.\xba\xe8\xa2\x8b\xffa\xfc\x17\x04\xec\x08\xf14\xc1\xbc\xe9\
\x00\x00\x00\x00IEND\xaeB`\x82\xb8\x05\xf3\x8b'
