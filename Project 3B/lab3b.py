#!/usr/bin/env python3
# lab3b
# NAME: Arnold Pfahnl, Cindi Dong
# EMAIL: ajpfahnl@gmail.com, cindidong@gmail.com
# ID: 305176399, 405126747

import sys
from collections import defaultdict

if __name__ == "__main__":
    # find file inputted from command line
    if len(sys.argv) != 2:
        print("usage: python3 lab3b <filename>", file=sys.stderr)
        sys.exit(1)
    in_file_name = sys.argv[1]

    errors = 0

    ############################
    # open file and parse data
    ############################
    sb_data = []  # superblock
    g_data = []  # group
    fb_data = set()  # free blocks
    fi_data = set()  # free inodes
    i_data = {}  # inode        key: i-num
    ind_data = defaultdict(list)  # indirect     key: i-num of owning file
    d_data = {}  # directory        key: i-num
    d_names = {}  # directory        key: i-num    value: name
    d_parent_data = {}  # directory     key: parent's i-num for ..    value: i-num for ..
    parent_data = {}  # directory       key: i-num   value: parent's i-num
    link_count = {}  # link count      key: i-num    value: number of links


    with open(in_file_name, 'r') as in_file:
        for line in in_file:
            line_list = line.strip().split(',')

            # parse free blocks
            if line_list[0] == 'BFREE':
                fb_data.add(int(line_list[1]))

            # parse free inodes
            elif line_list[0] == 'IFREE':
                fi_data.add(int(line_list[1]))

            # parse inodes
            # key:   i-num
            # value: [file_type, mode, owner, group, link count,
            #         ctime, mtime, atime, file size, # blocks, addresses]
            elif line_list[0] == 'INODE':
                i_data[int(line_list[1])] = line_list[2:]

            # parse indirect blocks
            # key:   i-num of owning file
            # value: [level of indirection, logical block offset, indirect block num,
            #         block num of referenced block], [...], [...], ...
            elif line_list[0] == 'INDIRECT':
                ind_data[int(line_list[1])].append(line_list[2:])

            # parse group
            elif line_list[0] == 'GROUP':
                g_data = [int(i) for i in line_list[1:]]
                # total_blocks      = g_data[0]
                # total_inodes      = g_data[1]
                first_inode_block = g_data[7]

            # parse superblock
            elif line_list[0] == 'SUPERBLOCK':
                sb_data = [int(i) for i in line_list[1:]]
                total_blocks, total_inodes, block_size, inode_size, \
                blocks_per_group, inodes_per_group, nonreserved_inode_start = \
                    sb_data

            # parse directory
            elif line_list[0] == 'DIRENT':
                # complete list of directories with key = i_num and value = parent's i-num
                d_data[int(line_list[3])] = int(line_list[1])
                # list of directory names
                d_names[int(line_list[3])] = line_list[6]
                # count links
                if int(line_list[3]) not in link_count:
                    link_count[int(line_list[3])] = 1
                else:
                    link_count[int(line_list[3])] = link_count[int(line_list[3])] + 1
                # key = parent's i-num for .. and value = i-num for ..
                if line_list[6] == "'..'":
                    d_parent_data[int(line_list[1])] = int(line_list[3])
                # key = i-num and value = parent's i-num
                elif line_list[6] != "'.'":
                    parent_data[int(line_list[3])] = int(line_list[1])


    nonreserved_block_start = int(first_inode_block + (total_inodes * inode_size) / block_size)
    if len(sb_data) == 0:
        print("No superblock data", file=sys.stderr)
        sys.exit(1)

    ############################
    # block consistency audit
    ############################
    refed_legal_blocks = defaultdict(list)
    # key: block
    # value: list of [indirection (decimal), i-num, offset]

    a_first = 10  # first address in i_data value
    levels = {0: "", 1: "INDIRECT ", 2: "DOUBLE INDIRECT ", 3: "TRIPLE INDIRECT "}

    ### check inodes ###
    for i_num, data in i_data.items():
        if len(data) > a_first:
            addresses = [int(i) for i in data[a_first:]]
            for offset, address in enumerate(addresses):
                if address == 0:
                    continue

                level_num = 0
                if offset == 12:
                    level_num = 1
                elif offset == 13:
                    level_num = 2
                    offset = 12 + 256
                elif offset == 14:
                    level_num = 3
                    offset = 12 + 256 + (256 * 256)
                level_str = levels[level_num]

                # check validity
                if (address < 0) or (address >= total_blocks):
                    print(f"INVALID {level_str}BLOCK {address} IN INODE {i_num} AT OFFSET {offset}")
                    errors += 1
                # check reserved
                elif (address < nonreserved_block_start):
                    print(f"RESERVED {level_str}BLOCK {address} IN INODE {i_num} AT OFFSET {offset}")
                    errors += 1
                # record if legal
                else:
                    refed_legal_blocks[address].append([level_num, i_num, offset])

    ### check indirect ###
    for i_num, data_list in ind_data.items():
        for data in data_list:
            level_num, offset, ind_block, refed_block = [int(i) for i in data]
            if refed_block == 0:
                continue
            level_str = levels[level_num]
            # check validity
            if (refed_block < 0) or (refed_block >= total_blocks):
                print(f"INVALID {level_str}BLOCK {ind_block} IN INODE {i_num} AT OFFSET {offset}")
                errors += 1
            # check reserved
            elif (refed_block < nonreserved_block_start):
                print(f"RESERVED {level_str}BLOCK {ind_block} IN INODE {i_num} AT OFFSET {offset}")
                errors += 1
            # record if legal
            else:
                refed_legal_blocks[refed_block].append([level_num, i_num, offset])

    ### check legal blocks ###
    for block in range(nonreserved_block_start, total_blocks):
        # determine conditions
        free = False
        refed = False
        if block in fb_data:
            free = True
        if block in refed_legal_blocks:
            refed = True

        # determine errors
        if (not free) and (not refed):
            print(f"UNREFERENCED BLOCK {block}")
            errors += 1
        elif (free) and (refed):
            print(f"ALLOCATED BLOCK {block} ON FREELIST")
            errors += 1

    ### check for duplicates ###
    for block, data_list in refed_legal_blocks.items():
        if len(data_list) > 1:
            for data in data_list:
                level_num, i_num, offset = data
                level_str = levels[level_num]
                print(f"DUPLICATE {level_str}BLOCK {block} IN INODE {i_num} AT OFFSET {offset}")
                errors += 1

    ############################
    # i-node allocation audits
    ############################
    allocated = []
    # checking the inode list for any that are also on the freelist
    for i_num in i_data:
        if i_num != 0:
            allocated.append(i_num)
            if i_num in fi_data:
                print(f"ALLOCATED INODE {i_num} ON FREELIST")
                errors += 1

    # checking all the inodes for unallocated inodes not on freelist
    for i_num in range(nonreserved_inode_start, total_inodes):
        if i_num not in allocated and i_num not in fi_data:
            print(f"UNALLOCATED INODE {i_num} NOT ON FREELIST")
            errors += 1

    # checking inode list for inodes with bad file types not in the freelist
    for i_num in i_data:
        if i_data.get(i_num)[0] == '?' and i_num not in fi_data:
            print(f"UNALLOCATED INODE {i_num} NOT ON FREELIST")
            errors += 1

    ############################
    # directory consistency audits
    ############################
    # check for invalid and unallocated inodes
    for dir_num in d_data:
        if dir_num < 1 or dir_num > total_inodes:
            print(f"DIRECTORY INODE {d_data.get(dir_num)} NAME {d_names.get(dir_num)} INVALID INODE {dir_num}")
            errors += 1
        elif dir_num not in allocated:
            print(f"DIRECTORY INODE {d_data.get(dir_num)} NAME {d_names.get(dir_num)} UNALLOCATED INODE {dir_num}")
            errors += 1

    # check if inode link count is equal to actual link count
    for i_num in i_data:
        # get link count in inode
        inode_link_count = int(i_data.get(i_num)[4])
        # get actual link count
        if i_num in link_count:
            real_link_count = link_count[i_num]
        else:
            real_link_count = 0
        if real_link_count != inode_link_count:
            print(f"INODE {i_num} HAS {real_link_count} LINKS BUT LINKCOUNT IS {inode_link_count}")
            errors += 1

    # check .
    for dir_num in d_data:
        if d_names.get(dir_num) == "'.'" and dir_num != d_data.get(dir_num):
            print(f"DIRECTORY INODE {d_data.get(dir_num)} NAME '.' LINK TO INODE {dir_num} SHOULD BE {d_data.get(dir_num)}")
            errors += 1

    # check ..
    for dir_num in d_parent_data:
        if dir_num == 2 and d_parent_data[dir_num] == 2:
            continue
        elif dir_num == 2:
            print(f"DIRECTORY INODE {dir_num} NAME '..' LINK TO INODE {d_parent_data[dir_num]} SHOULD BE {dir_num}")
            errors += 1
        elif dir_num not in parent_data:
            print(f"DIRECTORY INODE {d_parent_data[dir_num]} NAME '..' LINK TO INODE {dir_num} SHOULD BE {d_parent_data[dir_num]}")
            errors += 1
        elif d_parent_data[dir_num] != parent_data[dir_num]:
            print(f"DIRECTORY INODE {dir_num} NAME '..' LINK TO INODE {dir_num} SHOULD BE {parent_data[dir_num]}")
            errors += 1

    ############################
    # program exit
    ############################
    if errors > 0:
        sys.exit(2)
    else:
        sys.exit(0)
