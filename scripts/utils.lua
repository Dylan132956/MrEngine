

function strEndswith(s, suffix)
    return s:sub(-string.len(suffix)) == suffix
end

function appendArray(originalArray, toAppendArray)
    if type(toAppendArray) ~= 'table' or type(originalArray) ~= 'table' then
        return
    end

    for _, v in ipairs(toAppendArray) do
        table.insert(originalArray, v)
    end
end

function matchFiles(files)
    if type(files) ~= 'table' then
        return {}
    end

    local ret = {}
    for _, file in ipairs(files) do
        local matches = os.matchfiles(file)
        appendArray(ret, matches)
    end

    return ret
end

function mydofile(filename)
    local ret
    local status, f = pcall(loadfile, filename)
    if status and f then
        status, ret = pcall(f)
        if not status then
            print(string.format('==> mydofile(%s), ERROR: pcall func return: ' .. tostring(ret), filename))
        end
    else
        print(string.format('==> mydofile(%s), ERROR: pcall loadfile return: ' .. tostring(f), filename))
    end
    return ret
end