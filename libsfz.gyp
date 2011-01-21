{
    'target_defaults': {
        'include_dirs': [
            'include/all',
            'include/mac',
            'include/posix',
        ],
    },
    'targets': [
        {
            'target_name': 'libsfz',
            'type': '<(library)',
            'sources': [
                'src/all/sfz/Bytes.cpp',
                'src/all/sfz/CString.cpp',
                'src/all/sfz/Encoding.cpp',
                'src/all/sfz/Exception.cpp',
                'src/all/sfz/Format.cpp',
                'src/all/sfz/Formatter.cpp',
                'src/all/sfz/Integer.cpp',
                'src/posix/sfz/Io.cpp',
                'src/all/sfz/LinkedPtr.cpp',
                'src/posix/sfz/MappedFile.cpp',
                'src/all/sfz/NetworkBytes.cpp',
                'src/posix/sfz/Os.cpp',
                'src/posix/sfz/PosixFormatter.cpp',
                'src/all/sfz/PrintItem.cpp',
                'src/all/sfz/ReadItem.cpp',
                'src/mac/sfz/ReferenceCounted.cpp',
                'src/posix/sfz/ScopedFd.cpp',
                'src/all/sfz/Sha1.cpp',
                'src/all/sfz/String.cpp',
                'src/all/sfz/StringUtilities.cpp',
                'src/all/sfz/WriteItem.cpp',
            ],
            'direct_dependent_settings': {
                'include_dirs': [
                    'include/all',
                    'include/mac',
                    'include/posix',
                ],
            },
        },
        {
            'target_name': 'libsfz-tests',
            'type': 'executable',
            'sources': [
                'src/all/sfz/Bytes.test.cpp',
                'src/all/sfz/Encoding.test.cpp',
                'src/all/sfz/Foreach.test.cpp',
                'src/all/sfz/Formatter.test.cpp',
                'src/all/sfz/Io.test.cpp',
                'src/all/sfz/LinkedPtr.test.cpp',
                'src/all/sfz/Os.test.cpp',
                'src/all/sfz/PrintItem.test.cpp',
                'src/all/sfz/ReadItem.test.cpp',
                'src/all/sfz/Sha1.test.cpp',
                'src/all/sfz/String.test.cpp',
                'src/all/sfz/StringUtilities.test.cpp',
                'src/all/sfz/WriteItem.test.cpp',
            ],
            'dependencies': [
                ':libsfz',
                '<(DEPTH)/ext/googlemock/googlemock.gyp:gmock_main',
            ],
        },
    ],
}
