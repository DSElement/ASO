namespace TodoApp.Models
{
    /// <summary>
    /// Represents a single TODO item.
    /// </summary>
    public class TodoItem
    {
        public int Id { get; set; }

        public string? Title { get; set; }

        public string? Description { get; set; }

        public bool IsCompleted { get; set; }

        public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
    }
}
