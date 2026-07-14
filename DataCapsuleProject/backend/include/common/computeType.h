#pragma once

// Compute Type: edit this when add new compute type
enum ComputeType : unsigned char
{
    DATA_STATISTICS,
    SQL_OPERATION,
    DATABASE_OPERATION,
    MACHINE_LEARNING,
    MODEL_FINETUNE,
    PLAIN_TEXT,
    OFFICE_OPERATION
};

enum DataStatisticsType : unsigned char
{
    COUNT,
    MIN,
    MAX,
    AVG,
    SUM
};

enum SQLOperationType : unsigned char
{
    SELECT
};

enum ModelFinetuneType : unsigned char
{
    DeepSeek_R1_Distill_Qwen_1_5B,
    DeepSeek_R1_Distill_Qwen_7B,
    DeepSeek_R1_Distill_Qwen_14B,
    DeepSeek_R1_Distill_Qwen_32B,
    DeepSeek_R1
};

enum OfficeOperationType : unsigned char
{
    DOCX,
    XLSX,
    PPTX,
    DOC,
    XLS,
    PPT
};